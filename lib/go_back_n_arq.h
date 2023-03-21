#include <bits/stdc++.h>
#include <sys/socket.h> // for socket
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for inet_addr and htons
using namespace std;

// now how do we go about implementing go back N arq

// one function each to send and recieve frames (complimentary)

// one function each to send and recieve ack (complimentary)

// one function to handle the updation of the window

class go_back_n_arq{
    private:
        int frame_size; // frame size

        int window_size; // window size
        int start_index; // start index of the window

        mutex arq_lock; //locks entry to editing ack reflections

        vector<int> ack_values; // stores the ack values

        int sockfd; // socket file descriptor        

    public:

        go_back_n_arq(int frame_size, int window_size){


            this->frame_size = frame_size;
            this->window_size = window_size;
            this->start_index = 0;
            
            //flush the ack values(fill w zero)
            for(int i=0; i<window_size; i++){
                ack_values.push_back(0);
            }

            //initialize the socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);

            if (sockfd < 0) {
                cout << "Error in socket creation" << endl;
                exit(0);
            }
        }

        ~go_back_n_arq(){
            close(sockfd);
        }

        void connect(string ip, int port){

            struct sockaddr_in serv_addr;
            
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            int conn = connect(
                sockfd,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)
            );

            if (conn < 0) {
                cout << "Error in connection" << endl;
                exit(0);
            }
        }


        void send_file(file_url){

            int sendval;

            framing sendfile(file_url, frame_size);

            //need to send sendfile.frames

            thread recv_ack(recieve_ack);

            condition = true //TODO: end file condition

            while condition{
                
                //lock mutex
                arq_lock.lock();

                for(int i=start_index; i<start_index+window_size; i++){
                    sendval = send(sockfd, sendfile.frames[i], frame_size, 0);
                }

                //unlock mutex
                arq_lock.unlock();

            }

            recv_ack.join();

        }

        void recieve_file(target_url){
            //TODO
        }

        void send_ack(){
            //TODO
        }

        void recieve_ack(){

            // recieve ack values
            while true{

                int recieved_ack = recv(sockfd, ack_values, ack.values.size(), 0);
                
                // mutex lock
                arq_lock.lock();

                // update the start index
                for(int i=0; i<window_size; i++){
                    if(ack_values[i] == 1){
                        start_index++;
                        if (start_index == sendfile.frames.size()){
                            return;
                        }
                    }else{
                        break;
                    }
                }

                // flush the ack values
                for(int i=0; i<window_size; i++){
                    ack_values[i] = 0;
                }

                // mutex unlock
                arq_lock.unlock();
            }
        }
};