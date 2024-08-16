#include <string.h>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "log/log.h"
#include "master.h"
#ifdef _WIN32
#include "raw/serial_win32.h"
#else
#include "raw/serial_linux.h"
#endif

std::mutex serial_lock_;
std::condition_variable serial_cv_;
bool next_ = false;
bool close_ = false;
bool ready_ = false;

#define TransFrom()                      \
    std::string data;                    \
    std::cout << "my : ";                \
    std::getline(std::cin, data);        \
    if (data == "q" || data == "quit") { \
        break;                           \
    }                                    \
    next_ = false;                       \
    master.SendFrame((uint8_t*)data.c_str(), data.size());

void Slave(protocol::Master& master) {
    do {
        if (!next_) {
            std::unique_lock<std::mutex> lock(serial_lock_);
            serial_cv_.wait(lock, [] { return next_ || close_; });
        }
        if (close_) {
            break;
        }
        TransFrom();
    } while (true);
}

bool ReceivedHandler(uint8_t* msg, int size) {
    if (ready_) {
        std::string data((char*)msg, size);
        std::cout << "peer : " << data << std::endl;
        if (!next_) {
            std::lock_guard<std::mutex> lock(serial_lock_);
            next_ = true;
            serial_cv_.notify_one();
        }
    }

    return true;
}

SerialPortCommon* g_serial_;
protocol::Master* g_master_;
bool master_;

bool ConnectionEventHandler(protocol::ConnectionEvent ev) {
    switch (ev) {
        case protocol::CONNECTION_BROKEN:
            if (g_serial_->GetLastError() != raw::SERIAL_PORT_ERROR_NONE) {
                std::cout << "connection broken!" << std::endl;
                exit(-1);
            }
            if (master_) {
                g_master_->StartDT();
                g_master_->ResetDT();
            }
            return true;
        case protocol::CONNECTION_STARTDT:
            break;
        case protocol::CONNECTION_STARTDT_CONFIRMED:
            break;
        case protocol::CONNECTION_STOPDT:
            break;
        case protocol::CONNECTION_STOPDT_CONFIRMED:
            std::cout << "remote closed!" << std::endl;
            if (!close_) {
                std::lock_guard<std::mutex> lock(serial_lock_);
                close_ = true;
                serial_cv_.notify_all();
            }
            break;
        case protocol::CONNECTION_RESETDT:
            g_master_->SendFrame((uint8_t*)"hello, ready!", 13);
            ready_ = true;
            break;
        case protocol::CONNECTION_RESETDT_CONFIRMED:
            ready_ = true;
            break;
        default:
            break;
    }
    return false;
}

#define USAGE std::cout << "Usage: {-S|-M} [-D <serial_name>]" << std::endl
#ifdef _WIN32
#define SERIAL "COM1"
#else
#define SERIAL "/dev/ttyS0"
#endif

int main(int argc, char** argvs) {
    if (argc != 2 && argc != 4) {
    Err:
        USAGE;
        return 0;
    }

    if (argvs[1][0] == '-') {
        if (argvs[1][1] == 'S') {
            master_ = false;
        } else if (argvs[1][1] == 'M') {
            master_ = true;
        } else {
            goto Err;
        }
    }

    char dev_name[64];
    if (argc == 4) {
        if (argvs[2][0] == '-' && argvs[2][1] == 'D') {
#ifdef _WIN32
            sprintf_s(dev_name, "\\\\.\\%s", argvs[3]);
#else
            strcpy(dev_name, argvs[3]);
#endif
        }
    } else {
        strcpy(dev_name, SERIAL);
    }

    clog::g_logger_.init_logger(clog::Debug, "serial.log");

    // 创建串口实例
    SerialPortCommon serial(dev_name, 9600, 8, 'N', 1);
    g_serial_ = &serial;
    // 打开串口设备
    if (serial.Open()) {
        // 清除缓存
        serial.Discard();
        // 主从站实例
        protocol::Master master(&serial, {1.5, 5});
        g_master_ = &master;
        // 绑定接收回调
        master.SetRecviverHandler(ReceivedHandler);
        master.SetConnectionHandler(ConnectionEventHandler);
        // 启动串口监听
        master.Start();
        if (master_) {
            // 开始数据传输
            master.StartDT();
            // 重置数据传输
            master.ResetDT();
            Slave(master);
            // 停止数据传输
            master.StopDT();
        } else {
            Slave(master);
        }
        // 停止串口监听
        master.Stop();
    }

    return 0;
}
