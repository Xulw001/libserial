#include <iostream>
#ifdef _WIN32
#include "raw/serial_win32.h"
#else
#include "raw/serial_linux.h"
#endif
#include "master.h"

void Master(protocol::Master& master) {
    std::string data = {"This is Test Data from Master!"};
    master.SendFrame((uint8_t*)data.c_str(), data.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    getchar();
}

void Slave(protocol::Master& master) {
    std::string data = {"This is Test Data from Slave!"};
    master.SendFrame((uint8_t*)data.c_str(), data.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    getchar();
}

bool ReceivedHandler(void* parameter, uint8_t* msg, int size) {
    std::string data((char*)msg, size);
    std::cout << "data = " << data << std::endl;
    return true;
}

int main(int argc, char** argvs) {
    if (argc != 2) {
        return 0;
    }

    raw::SerialPortBase* base = new SerialPortCommon("COM1", 9600, 8, 'N', 1);
    if (base && base->Open()) {
        protocol::Master master(base);
        master.SetRecviverhandler(ReceivedHandler, NULL);
        master.Start();
        if (argvs[1][1] == 'S') {
            Slave(master);
        } else if (argvs[1][1] == 'M') {
            master.StartDT();
            Master(master);
            master.StopDT();
        }
        master.Stop();
    }

    return 0;
}