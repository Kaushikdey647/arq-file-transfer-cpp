#include "lib/framing.h"
// #include "lib/go_back_n_arq.h"

int main()
{
    string filename = "test.txt";
    
    framing f(filename, 64, 8);

    f.decode_frames("./");
    
    return 0;
}