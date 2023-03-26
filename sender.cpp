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

void send_file(string file_url){
    int sendval;

    encode_frames(file_url);

    cout << "\33[32m[DEBUG]: ENCODED FRAMES" << endl;

    //send array size:

    int array_size = frames.size();
    
    sendval = write(sockfd, &array_size, sizeof(array_size));

    cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << endl;

    cout << "\33[32m[DEBUG]: SENT ARRAY SIZE: " << array_size << endl;
    
    // sleep(1);

    //send frames

    while(true){

        for(int i=start_index; i<start_index+window_size && i < array_size; i++){
            char* frame = frames[i];
            // add hash

            cout << "\33[32m[DEBUG]: FRAME: " << i << endl;

            cout << "\33[32m HASH";

            // ADD CHECKSUM

            size_t hash = hash_fn(frame);

            memcpy(frame+frame_size+8, &hash, 8*sizeof(char));

            cout << "\33[32m INDEX";

            // ADD INDEX

            memcpy(frame+frame_size, &i, 8*sizeof(char));

            cout << "\33[32m FRAME" << endl;
            sendval = write(sockfd, frame, (frame_size+16)*sizeof(char));

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
                start_index++;
                if (start_index == frames.size()){
                    return;
                }
            }else{
                break;
            }
        }

        // terminate condition
        if (start_index == frames.size()){
            cout << "\33[32m[DEBUG]: TRANSMISSION COMPLETED: EXITING PROGRAM!" << endl;
            return;
        }

        cout << "\33[32m UPDATE ";

        // flush the ack values
        for(int i=0; i<window_size; i++){
            ack_values[i] = 0;
        }

        cout << "\33[32m FLUSH" << endl;

        // sleep(1);

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