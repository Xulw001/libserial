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

    /// @brief Initialize the environment of commucation
    /// NOTE: This function has to be called at begin
    void Start();

    /// @brief Release the environment of commucation
    /// NOTE: This function has to be called at end
    void Stop();

    /// @brief Start the commucation
    /// NOTE: This function has to be called after start
    void StartDT();

    /// @brief Reset the commucation
    /// NOTE: This function has to be called after startdt to notify the peer that it is ready
    void ResetDT();

    /// @brief Stop the commucation
    /// NOTE: This function has to be called before stop
    void StopDT();

    /// @brief Send data to peer by serial
    /// @param data the msg buffer to be send
    /// @param size the size of buffer
    void SendFrame(uint8_t* data, int size);

    /// @brief Register a callback handler for received connection event
    /// @param handler user provided callback handler function
    void SetConnectionHandler(ConnectionEventHandler handler);

    /// @brief Register a callback handler for received msg
    /// @param serial_receiver user provided callback handler function
    void SetRecviverHandler(MessageReceivedHandler serial_receiver);

   private:
    /// @brief Main thread function that runs the main loop.
    void MainThread();

    /// @brief Callback handler function for I-frame
    /// @param parameter provided parameter that is passed to the callback handler
    /// @param msg the msg received by serial
    /// @param size the size of msg
    /// @param more whether has more data
    bool DefaultRecviverHandler(uint8_t* msg, int size, bool more);

    /// @brief Callback handler function for U-frame
    /// @param frame frame type
    bool ConnectionHandler(UFrame frame);

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