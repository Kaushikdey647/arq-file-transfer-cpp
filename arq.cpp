#include "lib/arq.h"


int main(int argc, char const *argv[])
{
    //first element is hostname and second is port

    if( strcmp(argv[1], "s") == 0 ){
        arq server;

        //server
        server.arq_sock_listen(atoi(argv[2]));

        //recieve file
        server.recieve_file(argv[3]);

    }
    else if( strcmp(argv[1], "c") == 0 ){
        arq client;
        
        //client
        client.arq_sock_connect(argv[2], atoi(argv[3]));

        //send file
        client.send_file(argv[4]);
    }
    else{
        std::cout << "\33[31m[ERROR]: INVALID ARGUMENT " << argv[1] << std::endl;
        exit(0);
    }

    return 0;
}