#include "lib/framing.h"
#include <bits/stdc++.h>
#include <sys/socket.h> // for socket
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for inet_addr and htons
#include <unistd.h> //for close and shit
#include <iostream>

using namespace std;

int window_size = 8;

int frame_size = 128;

int start_index = 0;

int* ack_values = new int[window_size];

int sockfd = 0;

int connfd = 0;

mutex arq_lock;

vector<char*> frames;

void arq_sock_connect(const char* ip, int port ){
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if( sockfd < 0 ){
        cout << "[ERROR]: SOCKET CREATION FAILED" << endl;
        exit(0);
    }

    cout << "\33[32m[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << endl;

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    cout << "\33[32m[DEBUG]: SOCKET STRUCT CREATED" << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT PORT: " << port << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT IP: " << ip << endl;


    connfd = connect(
        sockfd,
        (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)
    );

    if (connfd < 0) {
        cout << "\33[31m[ERROR]: CONNECTION FAILED: " << connfd << endl;
        exit(0);
    }
}

void recieve_ack(){
    while(true){

        //lock mutex
        arq_lock.lock();

        cout << "\33[32m[DEBUG]: MUTEX LOCK" << endl;

        int ack = recv(connfd, ack_values, window_size, 0);

        cout << "\33[32m[DEBUG]: RECIEVED ACK VALUES" << endl;

        // update start_index
        for(int i=0; i<window_size; i++){
            if(ack_values[i] == 1){
                start_index++;
                if (start_index == frames.size()){
                    return;
                }
            }else{
                break;
            }
        }

        cout << "\33[32m[DEBUG]: UPDATED START INDEX" << endl;

        // flush the ack values
        for(int i=0; i<window_size; i++){
            ack_values[i] = 0;
        }

        cout << "\33[32m[DEBUG]: FLUSHED ACK VALUES" << endl;

        //unlock mutex
        arq_lock.unlock();

        cout << "\33[32m[DEBUG]: MUTEX UNLOCK" << endl;
    }
}

void send_file(string file_url){
    int sendval;

    framing sendfile(frame_size);

    sendfile.encode_frames(file_url);

    cout << "\33[32m[DEBUG]: ENCODED FRAMES" << endl;

    //copyy sendfile.frames to frames
    frames = sendfile.frames;


    //send array size:

    int array_size = sendfile.frames.size();

    sendval = send(connfd, &array_size, sizeof(int), 0);

    cout << "\33[32m[DEBUG]: SENT ARRAY SIZE: " << array_size << endl;

    //init thread

    thread recv_ack(recieve_ack);
    
    //send frames

    while(true){
            
        //lock mutex
        arq_lock.lock();

        for(int i=start_index; i<start_index+window_size; i++){
            char* frame = sendfile.frames[i];
            // add hash

            cout << "\33[32m[DEBUG]: ADDING HASH" << endl;

            sendfile.add_checksum(frame);

            cout << "\33[32m[DEBUG]: ADDING INDEX" << endl;

            sendfile.add_index(frame, i);

            cout << "\33[32m[DEBUG]: SENDING FRAME" << endl;

            sendval = send(connfd, sendfile.frames[i], frame_size+16, 0);
        }

        cout << "\33[32m[DEBUG]: MUTEX UNLOCK" << endl;

        //unlock mutex
        arq_lock.unlock();

        if( recv_ack.joinable() ){

            cout << "\33[32m[DEBUG]: JOINING THREAD" << endl;

            recv_ack.join();
            return;
        }
    }
}


int main(int argc, char const *argv[])
{
    //first element is hostname and second is port

    arq_sock_connect(argv[1],atoi(argv[2]));
    send_file("test.txt");

    return 0;
}