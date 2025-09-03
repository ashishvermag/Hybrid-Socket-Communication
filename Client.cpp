#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int MSG_LEN = 1024;

struct Message {
    int type;
    int length;
    char content[MSG_LEN];
};

void runRRClient(const char* serverIP, int serverPort) {
    while (true) {
        // TCP Phase
        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

        connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        // Send request
        Message request{1, 0, ""};
        send(tcpSocket, &request, sizeof(Message), 0);

        // Get UDP port
        Message response{};
        recv(tcpSocket, &response, sizeof(Message), 0);
        close(tcpSocket);

        int udpPort;
        memcpy(&udpPort, response.content, sizeof(int));

        // UDP Phase
        int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        serverAddr.sin_port = htons(udpPort);

        std::cout << "Enter message (type 'quit' to exit): ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "quit") break;

        Message udpMsg{3, static_cast<int>(input.size()), {}};
        strncpy(udpMsg.content, input.c_str(), MSG_LEN);

        sendto(udpSocket, &udpMsg, sizeof(Message), 0,
               (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        recvfrom(udpSocket, &udpMsg, sizeof(Message), 0, nullptr, nullptr);
        std::cout << "Server response: " << udpMsg.content << std::endl;

        close(udpSocket);
    }
}

void runFCFSClient(const char* serverIP, int serverPort) {
    while (true) {
        // Create TCP socket
        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcpSocket < 0) {
            std::cerr << "Error creating TCP socket" << std::endl;
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

        if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            close(tcpSocket);
            return;
        }

        std::cout << "Connected to the server. Requesting UDP port..." << std::endl;

        Message request{};
        request.type = 1;
        request.length = 0;
        strcpy(request.content, "Request for UDP port");

        if (send(tcpSocket, &request, sizeof(Message), 0) < 0) {
            std::cerr << "Failed to send request" << std::endl;
            close(tcpSocket);
            return;
        }

        Message response{};
        if (recv(tcpSocket, &response, sizeof(Message), 0) < 0) {
            std::cerr << "Failed to receive response" << std::endl;
            close(tcpSocket);
            return;
        }

        int udpPort;
        memcpy(&udpPort, response.content, sizeof(int));

        std::cout << "Received UDP port: " << udpPort << std::endl;

        close(tcpSocket);

        int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0) {
            std::cerr << "Error creating UDP socket" << std::endl;
            return;
        }

        serverAddr.sin_port = htons(udpPort);

        while (true) {
            Message udpMsg{};
            udpMsg.type = 3;

            std::cout << "Enter message to send via UDP (type 'quit' to exit): ";
            std::string userInput;
            std::getline(std::cin, userInput);

            strncpy(udpMsg.content, userInput.c_str(), MSG_LEN - 1);
            udpMsg.content[MSG_LEN - 1] = '\0';
            udpMsg.length = strlen(udpMsg.content);

            if (sendto(udpSocket, &udpMsg, sizeof(Message), 0,
                       (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Failed to send UDP message" << std::endl;
                close(udpSocket);
                return;
            }

            socklen_t addrLen = sizeof(serverAddr);
            if (recvfrom(udpSocket, &udpMsg, sizeof(Message), 0,
                         (struct sockaddr*)&serverAddr, &addrLen) < 0) {
                std::cerr << "Failed to receive UDP response" << std::endl;
                close(udpSocket);
                return;
            }

            std::cout << "Server response: " << udpMsg.content << std::endl;

            if (userInput == "quit") {
                break;
            }
        }

        close(udpSocket);

        std::cout << "Do you want to start a new session? (y/n): ";
        std::string choice;
        std::getline(std::cin, choice);

        if (choice != "y" && choice != "Y") {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <RR|FCFS>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = std::stoi(argv[2]);
    std::string policy = argv[3];

    if (policy == "RR") {
        runRRClient(serverIP, serverPort);
    } else if (policy == "FCFS") {
        runFCFSClient(serverIP, serverPort);
    } else {
        std::cerr << "Invalid policy. Use RR or FCFS." << std::endl;
        return 1;
    }

    return 0;
}



/*
#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
const int MSG_LEN = 1024;

struct Message {
    int type;
    int length;
    char content[MSG_LEN];
};

// Function to calculate throughput
void calculateThroughput(int k,size_t totalBytes, double durationInSeconds) {
    double throughput = totalBytes / durationInSeconds; // Bytes per second
    if(k==0)
        std::cout<<"\nFor TCP : \n";
    else
        std::cout<<"\nFor UDP : \n";
    std::cout << "Throughput: " << throughput << " Bytes/sec (" 
              << throughput * 8 << " bits/sec)" << std::endl;
}

void runRRClient(const char* serverIP, int serverPort) {
    while (true) {
        // TCP Phase
        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

        connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        auto startTime = std::chrono::high_resolution_clock::now(); // Start timer
        // Send request
        Message request{1, 0, ""};
        send(tcpSocket, &request, sizeof(Message), 0);

        auto endTime = std::chrono::high_resolution_clock::now(); // End timer
        // Calculate throughput
        size_t totalBytes = sizeof(Message); // Total bytes sent and received
        double durationInSeconds = std::chrono::duration<double>(endTime - startTime).count();
        
        calculateThroughput(0,totalBytes * 2, durationInSeconds); // Multiply by 2 for send + receive
        // Get UDP port
        Message response{};
        recv(tcpSocket, &response, sizeof(Message), 0);
        close(tcpSocket);

        int udpPort;
        memcpy(&udpPort, response.content, sizeof(int));

        // UDP Phase
        int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        serverAddr.sin_port = htons(udpPort);

        std::cout << "Enter message (type 'quit' to exit): ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "quit"){ 
            close(udpSocket);
            break;
        }

        Message udpMsg{3, static_cast<int>(input.size()), {}};
        strncpy(udpMsg.content, input.c_str(), MSG_LEN);

         startTime = std::chrono::high_resolution_clock::now(); // Start timer
        sendto(udpSocket, &udpMsg, sizeof(Message), 0,
               (struct sockaddr*)&serverAddr, sizeof(serverAddr));

                endTime = std::chrono::high_resolution_clock::now(); // End timer
               // Calculate throughput
         totalBytes = sizeof(Message); // Total bytes sent and received
         durationInSeconds = std::chrono::duration<double>(endTime - startTime).count();
        
        calculateThroughput(1,totalBytes * 2, durationInSeconds); // Multiply by 2 for send + receive
        recvfrom(udpSocket, &udpMsg, sizeof(Message), 0, nullptr, nullptr);
        std::cout << "Server response: " << udpMsg.content << std::endl;

        close(udpSocket);
    }
}

void runFCFSClient(const char* serverIP, int serverPort) {
    while (true) {
        // Create TCP socket
        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcpSocket < 0) {
            std::cerr << "Error creating TCP socket" << std::endl;
            return;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

        if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            close(tcpSocket);
            return;
        }

        std::cout << "Connected to the server. Requesting UDP port..." << std::endl;

        Message request{};
        request.type = 1;
        request.length = 0;
        strcpy(request.content, "Request for UDP port");

        auto startTime = std::chrono::high_resolution_clock::now(); // Start timer
        if (send(tcpSocket, &request, sizeof(Message), 0) < 0) {
            std::cerr << "Failed to send request" << std::endl;
            close(tcpSocket);
            return;
        }

        auto endTime = std::chrono::high_resolution_clock::now(); // End timer
        // Calculate throughput
        size_t totalBytes = sizeof(Message); // Total bytes sent and received
        double durationInSeconds = std::chrono::duration<double>(endTime - startTime).count();
        
        calculateThroughput(0,totalBytes * 2, durationInSeconds); // Multiply by 2 for send + receive
        Message response{};
        if (recv(tcpSocket, &response, sizeof(Message), 0) < 0) {
            std::cerr << "Failed to receive response" << std::endl;
            close(tcpSocket);
            return;
        }

        int udpPort;
        memcpy(&udpPort, response.content, sizeof(int));

        std::cout << "Received UDP port: " << udpPort << std::endl;

        close(tcpSocket);

        int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0) {
            std::cerr << "Error creating UDP socket" << std::endl;
            return;
        }

        serverAddr.sin_port = htons(udpPort);

        while (true) {
            Message udpMsg{};
            udpMsg.type = 3;

            std::cout << "Enter message to send via UDP (type 'quit' to exit): ";
            std::string userInput;
            std::getline(std::cin, userInput);

            strncpy(udpMsg.content, userInput.c_str(), MSG_LEN - 1);
            udpMsg.content[MSG_LEN - 1] = '\0';
            udpMsg.length = strlen(udpMsg.content);

            auto startTime = std::chrono::high_resolution_clock::now(); // Start timer
            if (sendto(udpSocket, &udpMsg, sizeof(Message), 0,
                       (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Failed to send UDP message" << std::endl;
                close(udpSocket);
                return;
            }

            auto endTime = std::chrono::high_resolution_clock::now(); // End timer
            // Calculate throughput
        size_t totalBytes = sizeof(Message); // Total bytes sent and received
        double durationInSeconds = std::chrono::duration<double>(endTime - startTime).count();
        
        calculateThroughput(1,totalBytes * 2, durationInSeconds); // Multiply by 2 for send + receive

            socklen_t addrLen = sizeof(serverAddr);
            if (recvfrom(udpSocket, &udpMsg, sizeof(Message), 0,
                         (struct sockaddr*)&serverAddr, &addrLen) < 0) {
                std::cerr << "Failed to receive UDP response" << std::endl;
                close(udpSocket);
                return;
            }

            std::cout << "Server response: " << udpMsg.content << std::endl;

            if (userInput == "quit") {
                break;
            }
        }

        close(udpSocket);

        std::cout << "Do you want to start a new session? (y/n): ";
        std::string choice;
        std::getline(std::cin, choice);

        if (choice != "y" && choice != "Y") {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <RR|FCFS>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = std::stoi(argv[2]);
    std::string policy = argv[3];

    if (policy == "RR") {
        runRRClient(serverIP, serverPort);
    } else if (policy == "FCFS") {
        runFCFSClient(serverIP, serverPort);
    } else {
        std::cerr << "Invalid policy. Use RR or FCFS." << std::endl;
        return 1;
    }

    return 0;
}
*/