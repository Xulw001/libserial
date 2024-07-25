#ifndef _MASTER_H
#define _MASTER_H

#include <thread>
#include <vector>

#include "protocol/frame.h"

namespace protocol {

typedef std::function<bool(void*, uint8_t*, int)> MessageReceivedHandler;

class Master {
   public:
    Master(SerialPortBase* serial_connection)
        : frame_(serial_connection) { running_ = false; }
    Master(SerialPortBase* serial_connection, const APCIParameters apci_parameters)
        : frame_(serial_connection, apci_parameters) { running_ = false; }
    ~Master() { ; }

    void Start();

    void Stop();

    void StartDT();

    void StopDT();

    void SendFrame(uint8_t* data, int size);

    void SetRecviverhandler(MessageReceivedHandler serial_receiver, void* param);

   private:
    void MainThread();

    bool DefaultRecviverHandler(void* parameter, uint8_t* msg, int size, bool more);

    void Store(uint8_t* data, int size);

    void SendData();

   private:
    Frame frame_;
    bool running_;
    std::thread work_;
    MessageReceivedHandler serial_receiver_;
    std::vector<uint8_t> buffer_;
};

}  // namespace protocol
#endif