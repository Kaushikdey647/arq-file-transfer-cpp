#include <vector>
#include <cstring>
#include <sys/socket.h> // for socket
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for inet_addr and htons
#include <netdb.h>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <unistd.h> //for close and shit
#include <iostream>
#include <functional>


class arq{

private:

    int window_size;
    int frame_size;
    int start_index;
    int* ack_values;
    int sockfd;
    int connfd;
    std::vector<char*> frames;
    std::hash<std::string> hash_fn;

public:

    arq(){
        window_size = 8;
        frame_size = 128;
        start_index = 0;
        ack_values = new int[window_size];
        sockfd = 0;
        connfd = 0;
    }

    void encode_frames(std::string file_url){

        // Clean Frames
        frames.clear();

        // OPEN FILE AS BINARY
        std::ifstream infile(file_url, std::ios::binary);

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

    void decode_frames(std::string path_url){

        //fetch the extension

        char* ext = new char[8];
        memcpy(ext, frames[0], 8*sizeof(char));

        // generate a random std::string
        int len = rand()%10;
        std::string rand_str(len, ' ');
        std::generate_n(rand_str.begin(), len, []() { return std::rand() % 10 + '0'; });

        // create a new file
        std::ofstream outfile(path_url + rand_str + "." + ext, std::ios::binary);

        //append the data from the binary array
        for(int i = 1; i < frames.size(); i++){
            outfile.write(frames[i], frame_size*sizeof(char));
        }

        // close the file
        outfile.close();

    }

    void arq_sock_listen(int port ){
        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if( sockfd < 0 ){
            std::cout << "Error in socket creation" << std::endl;
            exit(0);
        }

        std::cout << "\33[32m[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << std::endl;

        struct sockaddr_in serv_addr;

        //flush the struct
        memset(&serv_addr, 0, sizeof(serv_addr));

        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT CREATED" << std::endl;
        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << std::endl;
        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT PORT: " << port << std::endl;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = htons(INADDR_ANY);

        int conn = bind(
            sockfd,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)
        );


        if (conn < 0) {
            std::cout << "\33[31m[ERROR]: FAILED TO BIND" << std::endl;
            exit(0);
        }

        std::cout << "\33[32m[DEBUG]: SOCKET BINDED" << std::endl;

        std::cout << "\33[32m[DEBUG]: LISTENING SOCKFD: " << sockfd << std::endl;

        if( listen(sockfd, 1) < 0){
            std::cout << "\33[31m[ERROR]: FAILED TO LISTEN" << std::endl;
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
            std::cout << "\33[31m[ERROR]: FAILED TO ACCEPT" << std::endl;
            exit(0);
        }

        std::cout << "\33[32m[DEBUG]: CONNECTION ACCEPTED" << std::endl;
    }

    void arq_sock_connect(const char* ip, int port ){
        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if( sockfd < 0 ){
            std::cout << "\33[31m[ERROR]: SOCKET CREATION FAILED" << std::endl;
            exit(0);
        }

        std::cout << "\33[32m[DEBUG]: SOCKET CREATED WITH SOCKFD: " << sockfd << std::endl;

        // Getting Server

        struct sockaddr_in serv_addr;

        memset(&serv_addr, 0, sizeof(serv_addr));

        struct hostent *host = (struct hostent *) gethostbyname(ip);

        if (host == NULL) {
            std::cout << "\33[31m[ERROR]: HOST NOT FOUND" << std::endl;
            exit(0);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        memcpy(
            &serv_addr.sin_addr,
            host->h_addr_list[0],
            host->h_length
        );

        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT CREATED" << std::endl;
        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT SIZE: " << sizeof(serv_addr) << std::endl;
        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT PORT: " << port << std::endl;
        std::cout << "\33[32m[DEBUG]: SOCKET STRUCT IP: " << ip << std::endl;

        //Attempting connection to server

        connfd = connect(
            sockfd,
            (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)
        );

        if (connfd < 0) {
            std::cout << "\33[31m[ERROR]: CONNECTION FAILED: " << connfd << std::endl;
            exit(0);
        }
    }

    int send_file(std::string file_url){
        int sendval;
        int itrnum = 0;
        encode_frames(file_url);

        //refresh start_index and ack_values

        start_index = 0;
        
        for(int i=0; i<window_size; i++){
            ack_values[i] = 0;
        }

        std::cout << "\33[32m[DEBUG]: ENCODED FRAMES" << std::endl;

        //send array size:

        int array_size = frames.size();
        
        sendval = write(sockfd, &array_size, sizeof(array_size));

        std::cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << std::endl;

        std::cout << "\33[32m[DEBUG]: SENT ARRAY SIZE: " << array_size << std::endl;

        //send frame size:

        sendval = write(sockfd, &frame_size, sizeof(frame_size));

        std::cout << "\33[32m[DEBUG]: SENDVAL: " << sendval << std::endl;

        std::cout << "\33[32m[DEBUG]: SENT FRAME SIZE: " << frame_size << std::endl;
        
        // sleep(1);

        //send frames

        while(true){

            for(int i=start_index; i<start_index+window_size && i < array_size; i++){
                char* frame = new char[frame_size + 16]; // keeping window for appending hash and index

                // copy frame from vector
                memcpy(frame, frames[i], frame_size*sizeof(char));

                // add hash

                std::cout << "\33[32m [" << i <<  "]:";

                // ADD CHECKSUM

                size_t hash = hash_fn(std::string(frame,frame_size));

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

            std::cout << "\33[32m[DEBUG]: ACK READ VALUE: " << ack << std::endl;

            std::cout << "INDEX:\t";
            for(int i = start_index; (i < start_index+window_size) && (i < array_size); i++){
                std::cout << i << " ";
            }
            std::cout << "\nACK:\t";
            for(int i = 0; (i < window_size) && (i < array_size - start_index); i++){
                std::cout << ack_values[i] << " ";
            }
            std::cout << std::endl;

            // update start_index
            for(int i=0; i<window_size; i++){
                if(ack_values[i] == 1){
                    start_index += 1;
                }else{
                    break;
                }
            }

            std::cout << "\33[32m[DEBUG]: START INDEX: " << start_index << std::endl;
            std::cout << "\33[32m[DEBUG]: ARRAY SIZE: " << array_size << std::endl;

            // terminate condition
            if (start_index >= frames.size()){
                std::cout << "\33[32m[DEBUG]: TRANSMISSION COMPLETED: EXITING PROGRAM!" << std::endl;
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
        std::cout << "\33[32m[DEBUG]: READING ARRAY SIZE" << std::endl;
        sendval = read(connfd, &array_size, sizeof(array_size));
        std::cout << "\33[32m[DEBUG]: ARRAY SIZE RECIEVED: " << array_size << std::endl;

        // RECIEVE FRAME SIZE

        std::cout << "\33[32m[DEBUG]: READING FRAME SIZE" << std::endl;
        sendval = read(connfd, &frame_size, sizeof(frame_size));
        std::cout << "\33[32m[DEBUG]: FRAME SIZE RECIEVED: " << frame_size << std::endl;

        //clear and reinitialize the frames vector
        frames.clear();
        frames = std::vector<char*>(array_size);

        // sleep(1);
        
        
        while(true){

            //recieve packets
            char* packet = new char[frame_size + 16];
            sendval = read(connfd, packet, (frame_size+16)*sizeof(char));
            if (sendval < 0) {
                std::cout << "\33[31m[ERROR]: CONNECTION ERROR" << std::endl;
                exit(0);
            }
            //get index
            int index;
            memcpy(&index, packet+frame_size, 8*sizeof(char));
            std::cout << "\33[32m [" << index << "]: RECV";
            
            //verify checksum
            size_t act_hash, rec_hash; // actual hash, received hash
            act_hash = hash_fn( std::string(packet,frame_size) ); //get hash
            memcpy(&rec_hash, packet+frame_size+8, 8*sizeof(char));

            if(act_hash == rec_hash){
                
                std::cout << "\33[32m VER" << std::endl;
                frames[index] = packet;
                ack_values[index-start_index] = 1;
            }
            else{

                std::cout << "\33[31m DMG" << std::endl;

                //delete packet
                delete[] packet;
                ack_values[index-start_index] = 0;
            }

            // sleep(1);

            //SEND ACK AFTER A FULL WINDOW

            if( (1+index-start_index) == window_size || index == array_size-1 ){


                std::cout << "\33[32m[DEBUG]: WINDOW FULL, SENDING ACK: " << std::endl;
                std::cout << "INDEX:\t";
                for(int i = start_index; (i < start_index+window_size) && (i < array_size); i++){
                    std::cout << i << " ";
                }
                std::cout << "\nACK:\t";
                for(int i = 0; (i < window_size) && (i < array_size - start_index); i++){
                    std::cout << ack_values[i] << " ";
                }
                std::cout << std::endl;
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
                        std::cout << "\33[32m[DEBUG]: TRANSMISSION COMPLETED! WRITING TO FILE" << std::endl;
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

                std::cout << "\33[32m[DEBUG]: START_INDEX: " << start_index << std::endl;

            }


        }


    }
};