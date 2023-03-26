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

void arq_socket_listen(int port ){
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if( sockfd < 0 ){
        cout << "Error in socket creation" << endl;
        exit(0);
    }

    cout << "\33[32m[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << endl;

    struct sockaddr_in serv_addr;

    //flush the struct
    memset(&serv_addr, 0, sizeof(serv_addr));

    cout << "\33[32m[DEBUG]: SOCKET STRUCT CREATED" << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT PORT: " << port << endl;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);

    int conn = bind(
        sockfd,
        (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)
    );


    if (conn < 0) {
        cout << "\33[31m[ERROR]: FAILED TO BIND" << endl;
        exit(0);
    }

    cout << "\33[32m[DEBUG]: SOCKET BINDED" << endl;

    cout << "\33[32m[DEBUG]: LISTENING SOCKFD: " << sockfd << endl;

    if( listen(sockfd, 1) < 0){
        cout << "\33[31m[ERROR]: FAILED TO LISTEN" << endl;
        exit(0);
    }
}

void add_error(char* frame, int p){
    //choose a random number between 0 and 1
    float r = (float)rand()/(float)RAND_MAX;

    //if r is less than p, add error
    if(r < p){
        //choose a random index
        int index = rand() % frame_size;

        //flip the bit
        frame[index] = frame[index] ^ 1;
    }
}

void recieve_file(const char* folder_path){

    int sendval;

    framing sendfile(frame_size);

    // ACCEPT A NEW CONNECTION

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    
    connfd = accept(
        sockfd,
        (struct sockaddr*)&client_addr,
        &client_addr_size
    );
    
    if (connfd < 0) {
        cout << "\33[31m[ERROR]: FAILED TO ACCEPT" << endl;
        exit(0);
    }

    cout << "\33[32m[DEBUG]: CONNECTION ACCEPTED" << endl;

    // INITIALIZE AND RECIEVE

    int arr_size = 0;

    cout << "\33[32m[DEBUG]: READING ARRAY SIZE" << endl;
    
    sendval = read(connfd, &arr_size, sizeof(arr_size));

    cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << endl;

    cout << "\33[32m[DEBUG]: ARRAY SIZE RECIEVED: " << arr_size << endl;

    //initialize the frames vector

    frames = vector<char*>(arr_size);

    sleep(1);

    while(true){
        //recieve packets

        cout << "\33[32m[DEBUG]: READING PACKET" << endl;

        char* packet = new char[frame_size + 16];

        cout << "\33[32m[DEBUG]: PACKET CREATED" << endl;

        sendval = read(connfd, packet, (frame_size+16)*sizeof(char));

        cout << "\33[32m[DEBUG]: PACKET RECIEVED " << endl;

        //get index

        int index;

        memcpy(&index, packet+frame_size, 8*sizeof(char));

        cout << "\33[32m INDEX: " << index;

        //verify checksum and give acknowledgement

        if(sendfile.verify_checksum(packet)){
            
            cout << "\33[32m VERIFIED" << endl;

            frames[index] = packet;
            ack_values[index-start_index] = 1;
        }
        else{

            cout << "\33[31m DAMAGED" << endl;

            //delete packet
            delete[] packet;
            ack_values[index-start_index] = 0;
        }

        sleep(1);

        //SEND ACK AFTER A FULL WINDOW

        if( (1+index-start_index) == window_size ){


            cout << "\33[32m[DEBUG]: WINDOW FULL" << endl;


            cout << "INDEX:\t";
            for(int i = start_index; i < start_index+window_size; i++){
                cout << i << " ";
            }
            cout << "\nACK:\t";
            for(int i = start_index; i < start_index+window_size; i++){
                cout << ack_values[i] << " ";
            }
            cout << endl;

            cout << "\33[32m[DEBUG]: SENDING ACK" << endl;
            write(connfd, ack_values, window_size*sizeof(int));
            
            cout << "\33[32m[DEBUG]: ACK SENT" << endl;



            //reset the start_index from first zero
            for(int i=0; i<window_size; i++){
                if(ack_values[i] == 1){
                    start_index++;
                }
                else{
                    break;
                }
            }

            //reset the ack_values array
            for(int i=0; i<window_size; i++){
                ack_values[i] = 0;
            }

            cout << "\33[32m[DEBUG]: ACK VALUES RESET" << endl;

            cout << "\33[32m[DEBUG]: START INDEX RESET TO: " << start_index << endl;

        }

        //terminate condition

        if (index == arr_size-1){
            break;
        }
    }

    sendfile.frames = frames;

    cout << "\33[32m[DEBUG]: WRITING TO FILE" << endl;

    sendfile.decode_frames(folder_path);
}


int main(int argc, char* argv[]){

    if(argc != 2){
        cout << "Usage: ./reciever <port>" << endl;
        exit(0);
    }

    arq_socket_listen(atoi(argv[1]));

    recieve_file("./");

    return 0;
}