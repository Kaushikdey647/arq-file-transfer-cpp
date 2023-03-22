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

void arq_socket_listen(const char* ip, int port ){
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if( sockfd < 0 ){
        cout << "Error in socket creation" << endl;
        exit(0);
    }

    cout << "[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << endl;

    struct sockaddr_in serv_addr;

    cout << "[DEBUG]: SOCKET STRUCT CREATED" << endl;
    cout << "[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << endl;
    cout << "[DEBUG]: SOCKET STRUCT PORT: " << port << endl;
    cout << "[DEBUG]: SOCKET STRUCT IP: " << ip << endl;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    int conn = bind(
        sockfd,
        (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)
    );


    if (conn < 0) {
        cout << "[ERROR]: FAILED TO BIND" << endl;
        exit(0);
    }

    cout << "[DEBUG]: SOCKET BINDED" << endl;

    cout << "[DEBUG]: LISTENING SOCKFD: " << sockfd << endl;

    if( listen(sockfd, 1) < 0){
        cout << "[ERROR]: FAILED TO LISTEN" << endl;
        exit(0);
    }
}

void send_ack(){

    cout << "[DEBUG]: SENDING ACK" << endl;
    send(connfd, ack_values, window_size*sizeof(int), 0);
    
    cout << "[DEBUG]: ACK SENT" << endl;

    //reset the ack_values array
    for(int i=0; i<window_size; i++){
        ack_values[i] = 0;
    }

    cout << "[DEBUG]: ACK VALUES RESET" << endl;

    //reset the start_index from first zero
    for(int i=0; i<window_size; i++){
        if(ack_values[i] == 1){
            start_index++;
        }
        else{
            break;
        }
    }

    cout << "[DEBUG]: START INDEX RESET" << endl;
}

int accept_conn(){

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    
    connfd = accept(
        sockfd,
        (struct sockaddr*)&client_addr,
        &client_addr_size
    );
    
    if (connfd < 0) {
        cout << "[ERROR]: FAILED TO ACCEPT" << endl;
        exit(0);
    }

    cout << "[DEBUG]: CONNECTION ACCEPTED" << endl;

}

void recieve_file(const char* folder_path){

    int sendval;

    framing sendfile(frame_size);
    //recieve array size

    accept_conn();

    int arr_size = 0;

    cout << "[DEBUG]: READING ARRAY SIZE" << endl;

    sendval = read(connfd, &arr_size, sizeof(int));

    cout << "[DEBUG]: ARRAY SIZE RECIEVED: " << arr_size << endl;

    //initialize the frames vector

    frames = vector<char*>(arr_size);

    while(true){
        //recieve packets

        cout << "[DEBUG]: READING PACKET" << endl;

        char* packet = new char[frame_size+16];

        cout << "[DEBUG]: PACKET CREATED" << endl;

        sendval = read(connfd, packet, frame_size+16);

        //get index

        cout << "[DEBUG]: GETTING INDEX" << endl;

        int index = sendfile.get_index(packet);

        cout << "[DEBUG]: PACKET RECIEVED: " << index << endl;

        //verify checksum and give acknowledgement

        if(sendfile.verify_checksum(packet)){
            
            cout << "[DEBUG]: CHECKSUM VERIFIED" << endl;

            frames[index] = packet;
            ack_values[index-start_index] = 1;
        }
        else{

            cout << "[DEBUG]: CHECKSUM FAILED" << endl;

            //delete packet
            delete[] packet;
            ack_values[index-start_index] = 0;
        }

        if( (1+index-start_index) == window_size ){
            // send acknowledgement
            send_ack();
        }

        //terminate condition

        if (index == arr_size-1){
            break;
        }
    }

    sendfile.frames = frames;

    cout << "[DEBUG]: WRITING TO FILE" << endl;

    sendfile.decode_frames(folder_path);
}


int main(int argc, char* argv[]){

    if(argc != 3){
        cout << "Usage: ./reciever <ip> <port>" << endl;
        exit(0);
    }

    arq_socket_listen(argv[1], atoi(argv[2]));

    recieve_file("./");

    return 0;
}