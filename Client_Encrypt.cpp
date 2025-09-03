#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
// --- Original Client Logic (with modifications) ---
//================================================================================

const int MSG_LEN = 1024;
const std::string secretKey = "MySuperSecretKey"; // <-- IMPORTANT: Must be 16 chars

struct Message {
    int type;
    int length;
    char content[MSG_LEN];
};

void runClient(const char* serverIP, int serverPort) {
    while (true) {
        // TCP Phase to get UDP port
        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcpSocket < 0) { std::cerr << "Error creating TCP socket" << std::endl; return; }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

        if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "TCP Connection failed" << std::endl; close(tcpSocket); return;
        }
        std::cout << "Connected to server via TCP. Requesting UDP port..." << std::endl;

        Message request{1, 0, "Request for UDP port"};
        send(tcpSocket, &request, sizeof(Message), 0);

        Message response{};
        recv(tcpSocket, &response, sizeof(Message), 0);
        
        int udpPort;
        memcpy(&udpPort, response.content, sizeof(int));
        std::cout << "Received UDP port: " << udpPort << std::endl;
        close(tcpSocket);

        // UDP Phase for encrypted communication
        int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0) { std::cerr << "Error creating UDP socket" << std::endl; return; }
        serverAddr.sin_port = htons(udpPort);

        while (true) {
            std::cout << "\nEnter message to encrypt and send (type 'quit' to exit session): ";
            std::string userInput;
            std::getline(std::cin, userInput);

            try {
                // *** ENCRYPTION STEP ***
                std::string encryptedInput = xorEncrypt(userInput, secretKey);
                
                Message udpMsg{3, 0, {}};
                strncpy(udpMsg.content, encryptedInput.c_str(), MSG_LEN - 1);
                udpMsg.content[MSG_LEN - 1] = '\0';
                udpMsg.length = strlen(udpMsg.content);

                // *** DEBUG: SHOW ENCRYPTED MESSAGE TO BE SENT ***
                if (debug_mode) {
                    std::cout << "[DEBUG] Sending raw encrypted message: " << udpMsg.content << std::endl;
                }
                
                sendto(udpSocket, &udpMsg, sizeof(Message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

                // Wait for the server's response
                recvfrom(udpSocket, &udpMsg, sizeof(Message), 0, nullptr, nullptr);

                // *** DEBUG: SHOW RECEIVED ENCRYPTED MESSAGE ***
                if (debug_mode) {
                    std::cout << "[DEBUG] Received raw encrypted ACK: " << udpMsg.content << std::endl;
                }
                
                // *** DECRYPTION STEP ***
                std::string receivedCiphertext(udpMsg.content);
                std::string decryptedResponse = xorDecrypt(receivedCiphertext, secretKey);

                std::cout << "Server response (decrypted): \"" << decryptedResponse << "\"" << std::endl;
                
                if (userInput == "quit") {
                    break; 
                }
            } catch (const std::exception& e) {
                std::cerr << "An error occurred during encryption/decryption: " << e.what() << std::endl;
            }
        }

        close(udpSocket);

        std::cout << "\nSession finished. Start a new session? (y/n): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice != "y" && choice != "Y") {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> [-debug]" << std::endl;
        return 1;
    }

    std::vector<std::string> args;
    // Parse command line arguments, separating the debug flag
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-debug") == 0) {
            debug_mode = true;
        } else {
            args.push_back(argv[i]);
        }
    }

    if (args.size() != 2) {
        std::cerr << "Error: Incorrect number of arguments." << std::endl;
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> [-debug]" << std::endl;
        return 1;
    }

    if (debug_mode) {
        std::cout << "*** DEBUG MODE ENABLED ***" << std::endl;
    }

    const char* serverIP = args[0].c_str();
    int serverPort = std::stoi(args[1]);

    runClient(serverIP, serverPort);

    return 0;
}