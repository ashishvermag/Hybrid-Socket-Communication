#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

const int MSG_LEN = 1024;

struct Message {
    int type;
    int length;
    char content[MSG_LEN];
};

std::queue<int> requestQueue;
std::mutex queueMutex;
std::condition_variable queueCV;

void handleUDPCommunication(int udpPort) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udpAddr{};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = INADDR_ANY;
    udpAddr.sin_port = htons(udpPort);

    bind(udpSocket, (struct sockaddr*)&udpAddr, sizeof(udpAddr));

    Message udpMsg;
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    std::cout << "[RR] UDP socket bound to port " << udpPort << ". Waiting for message..." << std::endl;

    recvfrom(udpSocket, &udpMsg, sizeof(Message), 0,
             (struct sockaddr*)&clientAddr, &addrLen);

    if (udpMsg.type == 3) {
        std::cout << "[RR] UDP data received: " << udpMsg.content << std::endl;

        udpMsg.type = 4;
        strcpy(udpMsg.content, "ACK");
        sendto(udpSocket, &udpMsg, sizeof(Message), 0,
               (struct sockaddr*)&clientAddr, addrLen);

        std::cout << "[RR] ACK sent to client." << std::endl;
    }
    close(udpSocket);
    std::cout << "[RR] UDP socket closed." << std::endl;
}

void handleClient(int clientSocket) {
    Message request, response;

    recv(clientSocket, &request, sizeof(Message), 0);

    if (request.type == 1) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(8000, 8999);
        int udpPort = distrib(gen);

        std::cout << "[RR] TCP request received. Assigning UDP port: " << udpPort << std::endl;

        response.type = 2;
        response.length = sizeof(int);
        memcpy(response.content, &udpPort, sizeof(int));
        send(clientSocket, &response, sizeof(Message), 0);

        close(clientSocket);
        std::cout << "[RR] TCP connection closed after sending UDP port." << std::endl;

        handleUDPCommunication(udpPort);
    }
}

void scheduler() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [] { return !requestQueue.empty(); });

        int clientSocket = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[RR] Handling new client..." << std::endl;
        handleClient(clientSocket);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    std::cout << "[RR] Round Robin Server started on port " << port << std::endl;

    std::thread(scheduler).detach();

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(clientSocket);
        }
        queueCV.notify_one();
        std::cout << "[RR] New client connected and added to queue." << std::endl;
    }

    close(serverSocket);
    return 0;
}
