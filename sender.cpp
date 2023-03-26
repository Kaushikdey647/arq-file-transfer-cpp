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
float err_prob = 0.5;

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

            // ADD ERROR

            float r = (float)rand()/(float)RAND_MAX;

            if(r < err_prob){
                //choose a random index
                int err_ind = rand() % frame_size;
                //flip the bit
                frame[err_ind] = frame[err_ind] ^ 1;
                cout << "\33[31m ERR" << endl;
            }
            else{
                cout << "\33[32m NOERR" << endl;
            }

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

// ITERATORS START HERE

void iterate_thru_probs(){

    //create a logfile
    ofstream logfile("itr_prob_log.csv");

    logfile << "ERROR_PROBABILITY,TRANSMISSIONS" << endl;

    for(err_prob = 0.000; err_prob <= 0.950; err_prob += 0.001){
        logfile << err_prob << "," << send_file("test.txt") << endl;
    }

}

void iterate_thru_packet_size(){
    err_prob = 0.2;
    //create a logfile
    ofstream logfile("itr_packet_size_log.csv");

    logfile << "PACKET_SIZE,TRANSMISSIONS" << endl;

    for(frame_size = 8; frame_size <= 512; frame_size += 2){
        logfile << frame_size << "," << send_file("test.txt") << endl;
    }
}

void iterate_thru_packet_size_and_file_size(){
    err_prob = 0;
    //create logfile
    ofstream logfile("itr_packet_size_file_size_log.csv");

    logfile << "FILE_SIZE,PACKET_SIZE,TRANSMISSIONS" << endl;

    //ITERATE THROUGH FILE SIZE
    for(int i = 1; i < 64; i++){
        //create a binary file of size i KB
        ofstream file("tempfile", ios::binary);
        //fill the file with gibberish
        for(int j = 0; j < i*1024; j++){
            file << (char)rand();
        }
        file.close();
        //ITERATE THROUGH PACKET SIZE
        for(frame_size = 8; frame_size <= 512; frame_size += 8){
            logfile << i << "," << frame_size << "," << send_file("tempfile") << endl;
        }
    }
    //delete the file
    remove("tempfile");
}

void iterate_thru_file_size(){
    err_prob = 0;

    //create a logfile
    ofstream logfile("itr_file_size_log.csv");

    logfile << "FILE_SIZE,TRANSMISSIONS" << endl;

    for(int i = 1; i < 128; i++){
        //create a binary file of size i KB
        ofstream file("tempfile", ios::binary);
        //fill the file with gibberish
        for(int j = 0; j < i*1024; j++){
            file << (char)rand();
        }
        file.close();
        logfile << i << "," << send_file("tempfile") << endl;
    }

    //delete the file
    remove("tempfile");
}

int main(int argc, char const *argv[])
{
    //first element is hostname and second is port

    arq_sock_connect(argv[1],atoi(argv[2]));

    cout << "\33[32m[DEBUG]: CONNECTED TO SERVER, what are we logging against number of transmissions today?" << endl;

    cout << "\33[32m[DEBUG]: OPTIONS: bep, packet size, file size, packet size and file size" << endl;

    cout << "\33[32m[DEBUG]: ENTER LOG TYPE: " << endl;

    string log_type;
    cin >> log_type;
    

    if(log_type == "bep"){
        iterate_thru_probs();
    }else if(log_type == "packet size"){
        iterate_thru_packet_size();
    }else if(log_type == "file size"){
        iterate_thru_file_size();
    }else if(log_type == "packet size and file_size"){
        iterate_thru_packet_size_and_file_size();
    }else{
        cout << "\33[31m[ERROR]: INVALID LOG TYPE" << endl;
    }

    iterate_thru_packet_size_and_file_size();

    return 0;
}