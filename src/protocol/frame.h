#ifndef _FRAME_H
#define _FRAME_H

#include <functional>
#include <list>
#include <mutex>

#include "layer.h"

namespace protocol {

typedef std::function<bool(void*, uint8_t*, int, bool)> SerialReceivedHandler;

struct APCIParameters {
    float time_alive;
    float time_heart;
};

enum UFrame { START = 0x4,
              STARTC = 0x8,
              STOP = 0x10,
              STOPC = 0x20,
              TESTFR = 0x40,
              TESTFRC = 0x80 };

class Frame {
   public:
    Frame(SerialPortBase* serial_connection);
    Frame(SerialPortBase* serial_connection, const APCIParameters apci_parameters);
    ~Frame();

    /// @brief Register a callback handler for received
    /// @param serial_receiver user provided callback handler function
    /// @param param user provided parameter that is passed to the callback handler
    void SetRecviverhandler(SerialReceivedHandler serial_receiver, void* param) {
        serial_receiver_ = serial_receiver;
        serial_receiver_parameter_ = param;
    }

    /// @brief Receive a new message and run the protocol state machine(s).
    /// NOTE: This function has to be called frequently in order to send and receive messages to and from sides.
    /// @return
    bool Run();

    /// @brief Send a frame to sides
    /// @param data the frame buffer to be send
    /// @param size the size of frame buffer
    void SendFrame(uint8_t* data, int size);

    /// @brief Generate user specified u-frame
    /// @param type u-frame type
    /// @param frame_data the buffer to store frame data
    /// @return the size of frame
    static int PrepareUFrame(UFrame type, uint8_t* frame_data);

    /// @brief Generate i-frame by user data
    /// @param data user data buffer
    /// @param size the size of user data
    /// @param frame_data the buffer to store frame data
    /// @param more has more data
    /// @return the size of frame
    static int PrepareIFrame(uint8_t* data, int size, uint8_t* frame_data, bool more);

   private:
    /// @brief Callback handler function for layer
    /// @param parameter provided parameter that is passed to the callback handler
    /// @param msg the msg received by serial
    /// @param size the size of msg
    static void MessageHandler(void* parameter, uint8_t* msg, int size);

    /// @brief Reset the timeout of serial connection
    void ResetTimeout();

    /// @brief Handle the timeout event of serial connection
    /// @return false in case of timeout, false
    bool HandleTimeout();

    /// @brief Send the first frame in msg queue
    void SendSingleMessage();

   private:
    Layer frame_handler_;
    APCIParameters apci_parameters_;

   private:
    uint64_t next_heart_timeout_;
    int no_confirm_msg_;

   private:
    typedef struct sMsg Msg;
    std::list<Msg> msg_queue_;
    std::mutex queue_mutex_;

   private:
    SerialReceivedHandler serial_receiver_;
    void* serial_receiver_parameter_;
};

};  // namespace protocol
#endif