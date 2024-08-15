#ifndef _MASTER_H
#define _MASTER_H

#include <thread>
#include <vector>

#include "protocol/frame.h"

namespace protocol {

enum ConnectionEvent { CONNECTION_STARTDT,
                       CONNECTION_STARTDT_CONFIRMED,
                       CONNECTION_RESETDT,
                       CONNECTION_RESETDT_CONFIRMED,
                       CONNECTION_STOPDT,
                       CONNECTION_STOPDT_CONFIRMED,
                       CONNECTION_BROKEN,
};

typedef std::function<bool(ConnectionEvent)> ConnectionEventHandler;
typedef std::function<bool(uint8_t*, int)> MessageReceivedHandler;

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

    void ResetDT();

    void StopDT();

    void SendFrame(uint8_t* data, int size);

    void SetConnectionHandler(ConnectionEventHandler handler);

    void SetRecviverHandler(MessageReceivedHandler serial_receiver);

   private:
    void MainThread();

    bool DefaultRecviverHandler(uint8_t* msg, int size, bool more);

    bool ConnectionHandler(UFrame frame);

    void Store(uint8_t* data, int size);

    void SendData();

   private:
    Frame frame_;
    bool running_;
    std::thread work_;
    MessageReceivedHandler serial_receiver_;
    ConnectionEventHandler connection_ev_handler_;
    std::vector<uint8_t> buffer_;
};

}  // namespace protocol
#endif