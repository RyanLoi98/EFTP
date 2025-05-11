# 📁 UDP-Based File Transfer Protocol (EFTP)

**CPSC 441 – Computer Networks (Winter 2023)**
Author: Ryan Loi

## 📝 Overview

This project implements the **Easy File Transfer Protocol (EFTP)**, a lightweight and custom-designed file transfer protocol built on **UDP sockets**. Unlike TCP, UDP provides no reliability guarantees, so this project includes manual implementations of:

* Reliable delivery (ACKs, retransmissions, timeouts)
* Packet sequencing
* Authentication and session handling

This assignment demonstrates knowledge of low-level networking concepts, protocol design, and C socket programming using UDP.

---

## 🔧 Features

* 🔐 **Authentication phase** using username/password over UDP
* 🔄 **Reliable file transfers** (upload/download) with segment/block sequencing
* 🧠 **Client/server session management** using user assigned ports
* 💥 **Timeouts and retransmissions** on missing packets
* ❌ **Error handling** and graceful shutdown on protocol failures
* 📁 Support for both **text and binary files**
* 🧪 Tested with edge cases: 1KB, 8KB, and misaligned file sizes

---

## 🖥️ How to Compile

> Requires GCC or a compatible C compiler.
> Make sure you CD from the root directory

```bash
# Server
cd "Server Directory"
gcc eftpserver.c -o eftpserver

# Client
cd "Client Directory"
gcc eftpclient.c -o eftpclient
```
---

## 🚀 How to Run

### Server Syntax

```bash
./eftpserver [username:password] [listen_port] [working_directory]
```

Example:

```bash
./eftpserver alice:1234 9090 ./
```

### Client Syntax

```bash
./eftpclient [username:password@ip:port] [upload/download] [filename]
```

Example (download):

- To test download from the server to the client go into my testcases directory and get either a binary or text file (whatever you want to download) and place it into the server directory (or the working directory for the server). Then write the name of the file as the 3rd argument for the client. 

- For instance lets say I want to download the data.txt text file from my testcases, then I would take this data.txt file and place it into the server directory (or the working directory for the server). Then the 3rd argument would be data.txt for my client.

```bash
./eftpclient alice:1234@127.0.0.1:9090 download data.txt
```

Example (upload):

- To test uploading for the client to the server go into my testcases directory and get either a binary or text file (whatever you want to upload) and place it into the client directory (or the working directory for the client). Then write the name of the file as the 3rd argument for the client. 

- For instance lets say I want to upload the data.txt text file from my testcases, then I would take this data.txt file and place it into the server directory (or the working directory for the server). Then the 3rd argument would be data.txt for my client.


```bash
./eftpclient alice:1234@127.0.0.1:9090 upload data.txt
```

---

## 📸 Screenshot

Here is a screenshot of the file transfer in action:

![Screenshot](https://i.imgur.com/gv7ozxj.png)

---

## 📦 Protocol Design

### 🔐 AUTH (Authentication)

* Client sends username/password.
* Server responds with a random session ID (if valid).
* New socket/port is created for this session.

### 📄 RRQ / WRQ (Read/Write Request)

* `RRQ`: client requests file download.
* `WRQ`: client uploads a file to the server.

### 📦 DATA / ACK

* Files are broken into **blocks (8KB)**, each with **8 segments (1KB each)**.
* Client/Server **acknowledge each block** before moving to the next.
* Segments can arrive out of order and must be reassembled.

### ❌ ERROR

* Custom error messages are sent on invalid credentials, malformed requests, or failures.

---

## 🧪 Testing

Tested with:

* ✅ Small files (under 1KB)
* ✅ Exact 8KB files (to check end-of-transfer edge case)
* ✅ Binary files (e.g., `.jpg`, `.pdf`)
* ✅ Realistic packet loss simulations (via artificial drops)

---

## 📜 Portfolio Note

This project demonstrates manual implementation of transport-layer mechanisms typically handled by TCP. It showcases skills in protocol design, error handling, concurrency preparation, and real-world testing scenarios using UDP sockets.