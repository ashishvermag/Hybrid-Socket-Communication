# Hybrid Socket Communication in C++

This project demonstrates a hybrid communication model in C++. It combines TCP for reliable, stream-based communication and UDP for fast, connectionless datagrams. The system features a central server that manages multiple clients, each capable of communicating using either TCP or UDP.

## 🚀 Overview

The project consists of three main components:

1.  **Server:** A multithreaded server that listens for both TCP and UDP connections simultaneously using standard Berkeley sockets. It maintains a list of connected clients and can manage concurrent sessions.
2.  **Client:** A client application that can connect to the server via TCP for reliable communication.
3.  **Multithreading:** The server uses the POSIX threads (`pthreads`) library to create a dedicated thread for each connected TCP client, allowing it to handle multiple clients concurrently without blocking.

This architecture showcases how to build a flexible network application in C++ that leverages the distinct advantages of both TCP and UDP.

## ✨ Features

* **Dual Protocol Support:** Handles both TCP and UDP connections within a single server application.
* **Multithreaded Server:** Manages multiple client connections concurrently using `pthreads`.
* **Unique Client Identification:** Each client connection is handled in its own thread, identified by its socket descriptor.
* **Robust Connection Handling:** Gracefully handles client disconnections on the server side.

## 🛠️ Technologies Used

* **Language:** C++
* **Core APIs & Libraries:**
    * **Berkeley Sockets API:** (`<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`) for network communication.
    * **POSIX Threads (pthreads):** (`<pthread.h>`) for multithreading.
    * **Standard C++ Libraries:** For I/O, strings, and data structures.


## ▶️ How to Run the Project

You will need a C++ compiler (like `g++`) and the `make` utility installed on a Linux or other Unix-like system.

### 1. Compile the Project
Open a terminal, navigate to the project's root directory, and compile the server and client files. You must link the POSIX threads library (`pthread`) for the server.

**Compile the server:**
```bash
g++ server.cpp -o server -pthread
```

**Compile the client:**
```bash
g++ client.cpp -o client
```

### 2. Start the Server
In the same terminal, run the compiled server executable.

```bash
./server PORT_NO.
```
The server will start and print a message indicating it is listening for connections.

### 3. Run the Client
Open one or more **new** terminal windows and run the compiled client executable.

```bash
./client ip_address port_no. policy(FCFS/RR)
```
Each client instance will connect to the server and be ready to send and receive messages.

---
_Created by [Ashish Verma](https://github.com/ashishvermag)_
