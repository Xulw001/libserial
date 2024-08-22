#ifndef _FRAME_H
#define _FRAME_H

#include <functional>
#include <list>
#include <mutex>

#include "layer.h"

namespace protocol {

struct APCIParameters {
    float time_alive;
    float time_heart;
};

enum UFrame { START = 0x1,
              STARTC = 0x2,
              RESET = 0x4,
              RESETC = 0x8,
              STOP = 0x10,
              STOPC = 0x20,
              TESTFR = 0x40,
              TESTFRC = 0x80 };

typedef std::function<bool(UFrame)> UFrameHandler;
typedef std::function<bool(uint8_t*, int, bool)> IFrameHandler;

class Frame {
   public:
    Frame(SerialPortBase* serial_connection);
    Frame(SerialPortBase* serial_connection, const APCIParameters apci_parameters);
    ~Frame();

    /// @brief Register a callback handler for received i frame
    /// @param serial_receiver user provided callback handler function
    void SetIFrameHandler(IFrameHandler serial_receiver) {
        i_handler_ = serial_receiver;
    }

    /// @brief Register a callback handler for received u frame
    /// @param serial_receiver user provided callback handler function
    void SetUFrameHandler(UFrameHandler serial_receiver) {
        u_handler_ = serial_receiver;
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
    bool SendSingleMessage();

    void ResetAll();

   private:
    Layer frame_handler_;
    APCIParameters apci_parameters_;

   private:
    uint64_t next_heart_timeout_;
    int no_confirm_msg_;
    int send_frame_no_;
    int recv_frame_no_;

   private:
    typedef struct sMsg Msg;
    std::list<Msg> msg_queue_;
    std::mutex queue_mutex_;

   private:
    IFrameHandler i_handler_;
    UFrameHandler u_handler_;
};

};  // namespace protocol
#endif