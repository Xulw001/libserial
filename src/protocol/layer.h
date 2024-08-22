#ifndef _LAYER_H
#define _LAYER_H

#include <functional>

#include "raw/serial_base.h"

namespace protocol {

using raw::SerialPortBase;

typedef std::function<void(void*, uint8_t*, int)> SerialMessageHandler;

const uint8_t cBmark = 0xAA;
const uint8_t cImark = 0x96;
const uint8_t cUmark = 0x38;
const uint8_t cAmark = 0xe5;
const uint8_t cEmark = 0x10;

const uint8_t cIHeaderLength = 0x6;
const uint8_t cIFixedLength = 0x9;
const uint8_t cUFixedLength = 0x4;

class Layer {
   public:
    Layer(SerialPortBase* serial_connection) : serial_connection_(serial_connection) {
        message_timeout_ = 10;
        character_timeout_ = 300;
    }
    ~Layer() { ; }

    /// @brief Send a message of single frame
    /// @param msg data pointer to the frame.
    /// @param size data size of the frame
    /// @return true in case of success, false otherwise
    bool SendSingleMessage(uint8_t* msg, int size);

    /// @brief Read single frame from serial by registered callback
    /// @param buffer buffer to store the received data
    /// @param message_handler provided callback handler function
    /// @param parameter provided parameter that is passed to the callback handler
    void ReadNextMessage(uint8_t* buffer, SerialMessageHandler message_handler, void* parameter);

   private:
    /// @brief Read data from serial until count
    /// @param buffer buffer to store the received data
    /// @param count number of bytes to received
    /// @return number of received bytes
    int ReadBytesWithTimeout(uint8_t* buffer, int count);

   private:
    SerialPortBase* serial_connection_;

   private:
    int message_timeout_;
    int character_timeout_;
};

};  // namespace protocol

#endif