#include "lib/framing.h"

using namespace std;

int main(){
    int frame_size = 128;

    char* frame = new char[frame_size + 16];

    framing f(frame_size);

    f.add_checksum(frame);

    f.add_index(frame, 69);

    cout << "Packet: " << frame << endl;

    cout << "Index: " << f.get_index(frame) << endl;

    cout << "Checksum Verification: " << f.verify_checksum(frame) << endl;

    f.add_index(frame, 420);

    cout << "Index: " << f.get_index(frame) << endl;

    cout << "Checksum Verification: " << f.verify_checksum(frame) << endl;

    f.add_checksum(frame);

    cout << "Checksum Verification: " << f.verify_checksum(frame) << endl;
}