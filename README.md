# arq-file-transfer-cpp

## Description

What we created here is a go-back N protocol for file transfer. The sender sends a file to the receiver. The receiver receives the file and saves it to the disk. The sender and receiver communicate over a TCP connection. The sender and receiver are implemented in C++.
## Usage

### C++ CLI APPLICATION

- run `g++ arq.cpp -pthread -g -o bin/arq` ( don't forget to mkdir a bin folder first, I didn't have the time to take care of that )

- to expose port to client to recieve files `./bin/arq s <server-port> <file download path>`

- to upload a file to a server run `./bin/arq c <server-ip> <server-port> <file upload path>`
