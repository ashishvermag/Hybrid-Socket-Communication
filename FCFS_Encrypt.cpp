#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

// Global flag for debug mode
bool debug_mode = false;

//================================================================================
// --- Merged Encryption/Decryption Utility (from util.cpp) ---
//================================================================================

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const std::string& input) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const char* bytes_to_encode = input.c_str();
    size_t in_len = input.length();
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while((i++ < 3)) ret += '=';
    }
    return ret;
}

std::string base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;
    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++) char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; (i < 3); i++) ret += char_array_3[i];
            i = 0;
        }
    }
    if (i) {
        for (j = 0; j < i; j++) char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
        for (j = i; j <4; j++) char_array_4[j] = 0;
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }
    return ret;
}

std::string xorEncrypt(const std::string& plaintext, const std::string& key) {
    const size_t BLOCK_SIZE = 16;
    if (key.length() != BLOCK_SIZE) throw std::invalid_argument("Key must be exactly 16 characters long.");
    size_t padding_amount = BLOCK_SIZE - (plaintext.length() % BLOCK_SIZE);
    std::string padded_data = plaintext;
    padded_data.append(padding_amount, static_cast<char>(padding_amount));
    std::string encrypted_data(padded_data.size(), '\0');
    for (size_t i = 0; i < padded_data.length(); ++i) {
        encrypted_data[i] = padded_data[i] ^ key[i % BLOCK_SIZE];
    }
    return base64_encode(encrypted_data);
}

std::string xorDecrypt(const std::string& base64_ciphertext, const std::string& key) {
    const size_t BLOCK_SIZE = 16;
    if (key.length() != BLOCK_SIZE) throw std::invalid_argument("Key must be exactly 16 characters long.");
    std::string encrypted_bytes = base64_decode(base64_ciphertext);
    if (encrypted_bytes.length() % BLOCK_SIZE != 0) throw std::runtime_error("Invalid ciphertext length.");
    std::string decrypted_padded_bytes(encrypted_bytes.size(), '\0');
    for (size_t i = 0; i < encrypted_bytes.length(); ++i) {
        decrypted_padded_bytes[i] = encrypted_bytes[i] ^ key[i % BLOCK_SIZE];
    }
    size_t padding_amount = static_cast<unsigned char>(decrypted_padded_bytes.back());
    if (padding_amount == 0 || padding_amount > BLOCK_SIZE || decrypted_padded_bytes.length() < padding_amount) {
        throw std::runtime_error("Invalid padding amount.");
    }
    for(size_t i = 1; i <= padding_amount; ++i) {
        if (static_cast<unsigned char>(decrypted_padded_bytes[decrypted_padded_bytes.length() - i]) != padding_amount) {
            throw std::runtime_error("Invalid padding bytes found.");
        }
    }
    return decrypted_padded_bytes.substr(0, decrypted_padded_bytes.length() - padding_amount);
}

//================================================================================
// --- Original FCFS Server Logic (with modifications) ---
//================================================================================

const int MSG_LEN = 1024;
const std::string secretKey = "MySuperSecretKey"; // <-- IMPORTANT: Must be 16 chars

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

    std::cout << "[FCFS] UDP socket bound to port " << udpPort << ", waiting for encrypted client messages..." << std::endl;

    while (true) {
        memset(&udpMsg, 0, sizeof(Message));
        int bytesReceived = recvfrom(udpSocket, &udpMsg, sizeof(Message), 0,
                                     (struct sockaddr*)&clientAddr, &addrLen);

        if (bytesReceived <= 0) {
            std::cerr << "[FCFS] Error receiving UDP data" << std::endl;
            break;
        }

        if (udpMsg.type == 3) {
            // *** DEBUG: SHOW RECEIVED ENCRYPTED MESSAGE ***
            if (debug_mode) {
                std::cout << "\n[DEBUG] Received raw encrypted message: " << udpMsg.content << std::endl;
            }

            try {
                // *** DECRYPTION STEP ***
                std::string receivedCiphertext(udpMsg.content);
                std::string decryptedContent = xorDecrypt(receivedCiphertext, secretKey);
                
                std::cout << "[FCFS] Received and decrypted: \"" << decryptedContent << "\"" << std::endl;

                std::string responseContent;
                if (decryptedContent == "quit") {
                    std::cout << "[FCFS] Client requested to quit. Closing UDP connection." << std::endl;
                    responseContent = "Connection terminated";
                    udpMsg.type = 4;
                } else {
                    responseContent = "ACK: Message received";
                    udpMsg.type = 4;
                }
                
                // *** ENCRYPTION STEP ***
                std::string encryptedResponse = xorEncrypt(responseContent, secretKey);
                strncpy(udpMsg.content, encryptedResponse.c_str(), MSG_LEN -1);
                udpMsg.content[MSG_LEN - 1] = '\0';

                // *** DEBUG: SHOW ENCRYPTED MESSAGE TO BE SENT ***
                if (debug_mode) {
                    std::cout << "[DEBUG] Sending raw encrypted ACK: " << udpMsg.content << std::endl;
                }

                sendto(udpSocket, &udpMsg, sizeof(Message), 0, (struct sockaddr*)&clientAddr, addrLen);
                
                if (responseContent != "Connection terminated") {
                    std::cout << "[FCFS] Encrypted ACK sent to client." << std::endl;
                }

                if (decryptedContent == "quit") {
                    break; // Exit loop
                }

            } catch (const std::exception& e) {
                std::cerr << "[FCFS] Decryption failed: " << e.what() << std::endl;
            }
        }
    }

    close(udpSocket);
    std::cout << "[FCFS] UDP connection closed." << std::endl;
}

void handleClient(int clientSocket) {
    Message request, response;
    recv(clientSocket, &request, sizeof(Message), 0);

    if (request.type == 1) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(8000, 8999);
        int udpPort = distrib(gen);

        std::cout << "[FCFS] TCP request received. Assigning UDP port: " << udpPort << std::endl;

        response.type = 2;
        response.length = sizeof(int);
        memcpy(response.content, &udpPort, sizeof(int));
        send(clientSocket, &response, sizeof(Message), 0);

        close(clientSocket);
        std::cout << "[FCFS] TCP connection closed after sending UDP port." << std::endl;

        handleUDPCommunication(udpPort);
    }
}

void fcfsScheduler() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [] { return !requestQueue.empty(); });

        int clientSocket = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[FCFS] Handling new client..." << std::endl;
        handleClient(clientSocket);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [-debug]" << std::endl;
        return 1;
    }

    int port = -1;
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-debug") == 0) {
            debug_mode = true;
        } else {
            port = std::stoi(argv[i]);
        }
    }

    if (port == -1) {
        std::cerr << "Error: Port number not specified." << std::endl;
        std::cerr << "Usage: " << argv[0] << " <port> [-debug]" << std::endl;
        return 1;
    }

    if (debug_mode) {
        std::cout << "*** DEBUG MODE ENABLED ***" << std::endl;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    std::cout << "[FCFS] FCFS Encrypted Server started on port " << port << std::endl;

    std::thread(fcfsScheduler).detach();

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(clientSocket);
        }
        queueCV.notify_one();
        std::cout << "[FCFS] New client connected and added to queue." << std::endl;
    }

    close(serverSocket);
    return 0;
}