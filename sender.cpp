#include "lib/framing.h"
#include <bits/stdc++.h>
#include <sys/socket.h> // for socket
#include <sys/types.h>
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for inet_addr and htons
#include <netdb.h>
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
        cout << "\33[31m[ERROR]: SOCKET CREATION FAILED" << endl;
        exit(0);
    }

    cout << "\33[32m[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << endl;

    // Getting Server

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));

    struct hostent *host = (struct hostent *) gethostbyname(ip);

    if (host == NULL) {
        cout << "\33[31m[ERROR]: HOST NOT FOUND" << endl;
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    memcpy(
        &serv_addr.sin_addr,
        host->h_addr_list[0],
        host->h_length
    );

    cout << "\33[32m[DEBUG]: SOCKET STRUCT CREATED" << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT PORT: " << port << endl;
    cout << "\33[32m[DEBUG]: SOCKET STRUCT IP: " << ip << endl;

    //Attempting connection to server

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

    int ack = read(sockfd, ack_values, window_size);

    cout << "\33[32m[DEBUG]: ACK READ VALUE: " << ack << endl;

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

    cout << "\33[32m UPDATE ";

    // flush the ack values
    for(int i=0; i<window_size; i++){
        ack_values[i] = 0;
    }

    cout << "\33[32m FLUSH" << endl;
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
    
    sendval = write(sockfd, &array_size, sizeof(array_size));

    cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << endl;

    cout << "\33[32m[DEBUG]: SENT ARRAY SIZE: " << array_size << endl;
    
    sleep(1);

    //send frames

    while(true){

        for(int i=start_index; i<start_index+window_size; i++){
            char* frame = sendfile.frames[i];
            // add hash

            cout << "\33[32m[DEBUG]: FRAME: " << frame << endl;

            cout << "\33[32m HASH";

            sendfile.add_checksum(frame);

            cout << "\33[32m INDEX";

            sendfile.add_index(frame, i);

            cout << "\33[32m FRAME" << endl;

            cout << "\33[32m[DEBUG]: FRAME: " << frame << endl;

            cout << "\33[32m[DEBUG]: FRAME SIZE: " << frame_size+16 << endl;

            cout << "\33[32m[DEBUG]: FRAME INDEX: " << sendfile.get_index(frame) << endl;

            cout << "\33[32m[DEBUG]: HASH TRUE?: " << sendfile.verify_checksum(frame) << endl;

            sendval = write(sockfd, frame, (frame_size+16)*sizeof(char));

            sleep(1);
        }

        recieve_ack();

        sleep(1);

    }
}


int main(int argc, char const *argv[])
{
    //first element is hostname and second is port

    arq_sock_connect(argv[1],atoi(argv[2]));

    cout << "\33[32m[DEBUG]: CONNECTED TO SERVER, type in y to proceed: ";

    int x;

    cin >> x;

    send_file("test.txt");

    return 0;
}