#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

        mutex ack_edit; //locks entry to editing ack reflections

        vector<int> ack_values; // stores the ack values


    public:

        go_back_n_arq(int frame_size, int window_size){


            this->frame_size = frame_size;
            this->window_size = window_size;
            this->start_index = 0;
            
            //flush the ack values(fill w zero)

            
        }

        void send_window(){
            //TODO
        }

        void recieve_window(){
            //TODO
        }

        void send_ack(){
            //TODO
        }

        void recieve_ack(){
            // recieve ack values
            // mutex lock
            // update the start index
            // flush the ack values
            // mutex unlock
        }
};