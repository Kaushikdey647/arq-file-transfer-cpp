# arq-file-transfer-cpp

## Description

What we created here is a go-back N protocol for file transfer. The sender sends a file to the receiver. The receiver receives the file and saves it to the disk. The sender and receiver communicate over a TCP connection. The sender and receiver are implemented in C++.
## Usage

- run `g++ reciever.cpp -pthread -g -o bin/reciever && ./bin/reciever 8080`

- run `g++ sender.cpp -pthread -g -o bin/sender && ./bin/sender localhost 8080`