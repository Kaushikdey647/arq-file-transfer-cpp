#include <bits/stdc++.h>

using namespace std;

class framing{

    private:
        int frame_size;
        int file_size;
        string file_url;
        hash<string> hash_fn;
        int sockfd;
    public:
    
        vector<char *> frames;

        /**
         * @brief Construct a new framing object
         * 
         * @param frame_size
         * @param file_url 
         * 
         * Frame Breakdown: frame_size + 8 for hash + 8 for index
         */
        framing( string file_url, int frame_size){

            // ASSIGNMENTS
            this->frame_size = frame_size;
            this->file_url = file_url;
            // OPEN FILE AS BINARY
            ifstream infile(file_url, ios::binary);

            if (!infile.is_open())
            {
                cout << "Failed to open file\n";
                return;
            }

            // PUSH THE FILE NAME AND EXTENSION INTO THE VECTOR
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
        
        void add_checksum(char* frame){

            size_t hash = hash_fn(frame);

            // convert hash to char array
            memcpy(frame+frame_size, &hash, 8*sizeof(char));

        }

        bool verify_checksum(char* frame){

            size_t act_hash, rec_hash; // actual hash, received hash

            char* temp = new char[frame_size];
            
            memcpy(temp, frame, frame_size*sizeof(char));

            act_hash = hash_fn(temp);

            memcpy(&rec_hash, frame+frame_size, 8*sizeof(char));

            return act_hash == rec_hash;
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
};