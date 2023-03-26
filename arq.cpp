#include <bits/stdc++.h>
#include <sys/socket.h> // for socket
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

vector<char*> frames;

hash<string> hash_fn;

void encode_frames(string file_url){

    // Clean Frames
    frames.clear();

    // OPEN FILE AS BINARY
    ifstream infile(file_url, ios::binary);

    if (!infile.is_open())
    {
        throw "File not found";
    }

    // PUSH EXTENSION INTO THE VECTOR
    char* ext = new char[frame_size + 16];
    memcpy(ext, file_url.substr(file_url.find_last_of(".")+1).c_str(), frame_size*sizeof(char));
    
    frames.push_back(ext);


    // READ INTO THE BUFFER AND PUSH TO THE VECTOR
    char* buffer = new char[ frame_size ];

    while (infile.read(buffer, frame_size*sizeof(char)))
    {
        char* new_buffer = new char[frame_size + 16]; // keeping window for appending hash
        memcpy(new_buffer, buffer, frame_size*sizeof(char));
        frames.push_back(new_buffer);
    }

    // CLOSE THE FILE
    infile.close();
}

void decode_frames(string path_url){

    //fetch the extension

    char* ext = new char[8];
    memcpy(ext, frames[0], 8*sizeof(char));

    // generate a random string
    string rand_str = to_string(rand());

    // create a new file
    ofstream outfile(path_url + rand_str + "." + ext, ios::binary);

    //append the data from the binary array
    for(int i = 1; i < frames.size(); i++){
        outfile.write(frames[i], frame_size*sizeof(char));
    }

    // close the file
    outfile.close();

}

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
}

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

int send_file(string file_url){
    int sendval;
    int itrnum = 0;
    encode_frames(file_url);

    //refresh start_index and ack_values

    start_index = 0;
    
    for(int i=0; i<window_size; i++){
        ack_values[i] = 0;
    }

    cout << "\33[32m[DEBUG]: ENCODED FRAMES" << endl;

    //send array size:

    int array_size = frames.size();
    
    sendval = write(sockfd, &array_size, sizeof(array_size));

    cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << endl;

    cout << "\33[32m[DEBUG]: SENT ARRAY SIZE: " << array_size << endl;

    //send frame size:

    sendval = write(sockfd, &frame_size, sizeof(frame_size));

    cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << endl;

    cout << "\33[32m[DEBUG]: SENT FRAME SIZE: " << frame_size << endl;
    
    // sleep(1);

    //send frames

    while(true){

        for(int i=start_index; i<start_index+window_size && i < array_size; i++){
            char* frame = new char[frame_size + 16]; // keeping window for appending hash and index

            // copy frame from vector
            memcpy(frame, frames[i], frame_size*sizeof(char));

            // add hash

            cout << "\33[32m [" << i <<  "]:";

            // ADD CHECKSUM

            size_t hash = hash_fn(string(frame,frame_size));

            memcpy(frame+frame_size+8, &hash, 8*sizeof(char));

            // ADD INDEX

            memcpy(frame+frame_size, &i, 8*sizeof(char));

            sendval = write(sockfd, frame, (frame_size+16)*sizeof(char));
            
            delete[] frame;
            
            itrnum += 1;
            // sleep(1);
        }

        //RECIEVE ACK

        int ack = read(sockfd, ack_values, window_size*sizeof(int));

        cout << "\33[32m[DEBUG]: ACK READ VALUE: " << ack << endl;

        cout << "INDEX:\t";
        for(int i = start_index; (i < start_index+window_size) && (i < array_size); i++){
            cout << i << " ";
        }
        cout << "\nACK:\t";
        for(int i = 0; (i < window_size) && (i < array_size - start_index); i++){
            cout << ack_values[i] << " ";
        }
        cout << endl;

        // update start_index
        for(int i=0; i<window_size; i++){
            if(ack_values[i] == 1){
                start_index += 1;
            }else{
                break;
            }
        }

        cout << "\33[32m[DEBUG]: START INDEX: " << start_index << endl;
        cout << "\33[32m[DEBUG]: ARRAY SIZE: " << array_size << endl;

        // terminate condition
        if (start_index >= frames.size()){
            cout << "\33[32m[DEBUG]: TRANSMISSION COMPLETED: EXITING PROGRAM!" << endl;
            return itrnum;
        }

        // flush the ack values
        for(int i=0; i<window_size; i++){
            ack_values[i] = 0;
        }

    }
}

void recieve_file(const char* folder_path){

    int sendval;

    // ACCEPT A NEW CONNECTION

    //refresh the start_index and ack_values
    start_index = 0;
    delete[] ack_values;
    ack_values = new int[window_size];



    // INITIALIZE AND RECIEVE

    int array_size = 0;
    cout << "\33[32m[DEBUG]: READING ARRAY SIZE" << endl;
    sendval = read(connfd, &array_size, sizeof(array_size));
    cout << "\33[32m[DEBUG]: ARRAY SIZE RECIEVED: " << array_size << endl;

    // RECIEVE FRAME SIZE

    cout << "\33[32m[DEBUG]: READING FRAME SIZE" << endl;
    sendval = read(connfd, &frame_size, sizeof(frame_size));
    cout << "\33[32m[DEBUG]: FRAME SIZE RECIEVED: " << frame_size << endl;

    //clear and reinitialize the frames vector
    frames.clear();
    frames = vector<char*>(array_size);

    // sleep(1);
    
    
    while(true){

        //recieve packets
        char* packet = new char[frame_size + 16];
        sendval = read(connfd, packet, (frame_size+16)*sizeof(char));
        if (sendval < 0) {
            cout << "\33[31m[ERROR]: CONNECTION ERROR" << endl;
            exit(0);
        }
        //get index
        int index;
        memcpy(&index, packet+frame_size, 8*sizeof(char));
        cout << "\33[32m [" << index << "]: RECV";
        
        //verify checksum
        size_t act_hash, rec_hash; // actual hash, received hash
        act_hash = hash_fn( string(packet,frame_size) ); //get hash
        memcpy(&rec_hash, packet+frame_size+8, 8*sizeof(char));

        if(act_hash == rec_hash){
            
            cout << "\33[32m VER" << endl;
            frames[index] = packet;
            ack_values[index-start_index] = 1;
        }
        else{

            cout << "\33[31m DMG" << endl;

            //delete packet
            delete[] packet;
            ack_values[index-start_index] = 0;
        }

        // sleep(1);

        //SEND ACK AFTER A FULL WINDOW

        if( (1+index-start_index) == window_size || index == array_size-1 ){


            cout << "\33[32m[DEBUG]: WINDOW FULL, SENDING ACK: " << endl;
            cout << "INDEX:\t";
            for(int i = start_index; (i < start_index+window_size) && (i < array_size); i++){
                cout << i << " ";
            }
            cout << "\nACK:\t";
            for(int i = 0; (i < window_size) && (i < array_size - start_index); i++){
                cout << ack_values[i] << " ";
            }
            cout << endl;
            write(connfd, ack_values, window_size*sizeof(int));


            //terminate condition

            if (index == array_size-1){
                
                bool ack_recieved = true;

                //All ACKS recieved
                for(int i=0; (i < window_size) && (i < array_size - start_index); i++){
                    if(ack_values[i] == 0){
                        ack_recieved = false;
                        break;
                    }
                }
                
                if(ack_recieved){
                    cout << "\33[32m[DEBUG]: TRANSMISSION COMPLETED! WRITING TO FILE" << endl;
                    if(folder_path != ""){
                        decode_frames(folder_path);
                    }
                    return;
                }
            }

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

            cout << "\33[32m[DEBUG]: START_INDEX: " << start_index << endl;

        }


    }


}

int main(int argc, char const *argv[])
{
    //first element is hostname and second is port

    if(argv[1] == "s"){
        //server
        arq_socket_listen(atoi(argv[2]));

        //recieve file
        recieve_file(argv[3]);
    }
    else if(argv[1] == "c"){
        //client
        arq_socket_connect(argv[2], atoi(argv[3]));

        //send file
        send_file(argv[4]);
    }
    else{
        cout << "\33[31m[ERROR]: INVALID ARGUMENTS" << endl;
        exit(0);
    }

    return 0;
}