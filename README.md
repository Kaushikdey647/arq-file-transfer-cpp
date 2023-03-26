# arq-file-transfer-cpp

## Description

What we created here is a go-back N protocol for file transfer. The sender sends a file to the receiver. The receiver receives the file and saves it to the disk. The sender and receiver communicate over a TCP connection. The sender and receiver are implemented in C++.
## Usage

### C++ CLI APPLICATION

- run `g++ arq.cpp -pthread -g -o bin/arq`

- run `./bin/arq s 8080 ./`

- run `./bin/arq c localhost 8080 ./test.txt`

### BUILD PYTHON MODULE

- run `python setup.py build_ext --inplace`

- run `python setup.py install`
