#include <iostream>
#ifdef _WIN32
#include "raw/serial_win32.h"
#else
#include "raw/serial_linux.h"
#endif
#include "master.h"

void Master(protocol::Master& master) {
    std::string data = {"This is Test Data from Master!"};
    // 发送数据
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

    // 创建串口实例
    raw::SerialPortBase* base = new SerialPortCommon("COM1", 9600, 8, 'N', 1);
    // 打开串口设备
    if (base && base->Open()) {
        // 主从站实例
        protocol::Master master(base);
        // 绑定接收回调
        master.SetRecviverhandler(ReceivedHandler, NULL);
        // 启动串口监听
        master.Start();
        if (argvs[1][1] == 'S') {
            Slave(master);
        } else if (argvs[1][1] == 'M') {
            // 发送启动报文
            master.StartDT();
            Master(master);
            // 发送停止报文
            master.StopDT();
        }
        // 停止串口监听
        master.Stop();
    }

    return 0;
}