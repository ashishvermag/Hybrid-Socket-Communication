# Client-Server Communication with FCFS & RR Policies

This C++ project implements a secure client-server communication framework to demonstrate and compare two server-side scheduling policies: **First-Come, First-Served (FCFS)** and a single-exchange **Round Robin (RR)**.

The system uses a hybrid TCP/UDP approach, where TCP handles the initial connection setup and UDP is used for subsequent data transfer. All communication during the UDP phase is secured using a symmetric XOR encryption scheme combined with Base64 encoding to ensure confidentiality.

---

## 📜 Features

* **Client-Server Architecture:** A robust model with separate executables for the server and client.
* **Hybrid TCP/UDP Protocol:** Uses TCP for reliable connection setup and UDP for fast, message-based data transfer.
* **Dynamic Port Allocation:** The server dynamically assigns a unique UDP port for each client session.
* **Symmetric Encryption:** Implements a custom XOR encryption with a 16-byte key to protect message confidentiality.
* **Base64 Encoding:** Encodes the encrypted binary data into a text-based format, ensuring safe transmission.
* **FCFS & RR Server Policies:** Provides two distinct server behaviors for handling client sessions.
* **Throughput Measurement Client:** Includes a specialized client to measure network performance.
* **Debug Mode:** An optional `-debug` flag to print raw encrypted messages for troubleshooting.

---

## ⚙️ How It Works

The communication flow is designed in two main phases:

### Phase 1: TCP Handshake & Port Negotiation
1.  The **Server** starts and listens for incoming TCP connections on a specified port.
2.  A **Client** initiates a TCP connection to the server's main port.
3.  The server accepts the connection, generates a random UDP port, and sends this port number back to the client.
4.  Both the client and server close the TCP connection.

### Phase 2: Encrypted UDP Communication
1.  The server opens a new UDP socket and binds it to the port it just assigned.
2.  The client opens a UDP socket and begins sending messages to the server at the assigned port.
3.  **Encryption:** Before sending, the client encrypts the user's message using the `xorEncrypt` function and sends the resulting Base64 string.
4.  **Decryption:** The server receives the UDP datagram, decodes the Base64 string, and decrypts the content using the `xorDecrypt` function to retrieve the original plaintext.
5.  The server processes the message and sends back an encrypted acknowledgment (ACK).
6.  This session continues until the client sends a `quit` message.

---

## 🔐 Encryption Scheme

The encryption is a simple but effective stream cipher implemented as follows:

* **Padding:** The plaintext message is padded to be a multiple of the 16-byte block size.
* **XOR Operation:** Each byte of the padded data is XORed with a byte from a 16-character secret key. The key is repeated as necessary for the length of the message.
* **Base64 Encoding:** The resulting binary ciphertext is encoded into a standard Base64 string. This makes the data safe for transmission as plain text and avoids issues with non-printable characters.

Decryption reverses this process: Base64 decoding followed by the same XOR operation.

---

## 💻 Compilation

You will need a C++ compiler (`g++`) and a POSIX-compliant operating system (Linux, macOS) for the socket and threading libraries.

```bash
# Compile the FCFS Encrypted Server
g++ FCFS_Encrypt.cpp -o fcfs_server -std=c++11 -pthread

# Compile the RR Encrypted Server (assuming RR_Encrypt.cpp)
g++ RR_Encrypt.cpp -o rr_server -std=c++11 -pthread

# Compile the Encrypted Client (assuming Client_Encrypt.cpp)
g++ Client_Encrypt.cpp -o client -std=c++11

# Compile the Encrypted Throughput Client (assuming throughput_Encrypt.cpp)
g++ throughput_Encrypt.cpp -o throughput_client -std=c++11
