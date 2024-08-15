#include "master.h"

#include "log/log.h"

namespace protocol {

const uint16_t frame_size = 0x1000;
const uint16_t frame_limit = frame_size - cIFixedLength;

void Master::StartDT() {
    uint8_t frame[256];
    int len = Frame::PrepareUFrame(START, frame);
    if (len) {
        frame_.SendFrame(frame, len);
    }
}

void Master::ResetDT() {
    uint8_t frame[256];
    int len = Frame::PrepareUFrame(RESET, frame);
    if (len) {
        frame_.SendFrame(frame, len);
    }
}

void Master::StopDT() {
    uint8_t frame[256];
    int len = Frame::PrepareUFrame(STOP, frame);
    if (len) {
        frame_.SendFrame(frame, len);
    }
}

void Master::SendFrame(uint8_t* data, int size) {
    int pos = 0, offset = frame_limit;
    uint8_t buffer[frame_size];
    while (pos < size) {
        if (size - pos > offset)
            offset = frame_limit;
        else
            offset = size - pos;

        int len = protocol::Frame::PrepareIFrame(data + pos, offset, buffer, pos + offset < size);
        frame_.SendFrame(buffer, len);
        pos += offset;
    }
    qDebug << "send data len = " << size;
}

void Master::SetRecviverHandler(MessageReceivedHandler serial_receiver) {
    serial_receiver_ = serial_receiver;
}

void Master::SetConnectionHandler(ConnectionEventHandler handler) {
    connection_ev_handler_ = handler;
}

bool Master::ConnectionHandler(UFrame frame) {
    if (connection_ev_handler_) {
        switch (frame) {
            case START:
                connection_ev_handler_(CONNECTION_STARTDT);
                break;
            case STARTC:
                connection_ev_handler_(CONNECTION_STARTDT_CONFIRMED);
                break;
            case RESET:
                connection_ev_handler_(CONNECTION_RESETDT);
                break;
            case RESETC:
                connection_ev_handler_(CONNECTION_RESETDT_CONFIRMED);
                break;
            case STOP:
                connection_ev_handler_(CONNECTION_STOPDT);
                break;
            case STOPC:
                connection_ev_handler_(CONNECTION_STOPDT_CONFIRMED);
                break;
            default:
                break;
        }
    }
    return true;
}

bool Master::DefaultRecviverHandler(uint8_t* msg, int size, bool more) {
    buffer_.insert(buffer_.end(), msg, msg + size);
    if (!more) {
        serial_receiver_(buffer_.data(), (int)buffer_.size());
        qDebug << "recv data len = " << buffer_.size();
        buffer_.clear();
    }
    return true;
}

void Master::Start() {
    if (!work_.joinable()) {
        frame_.SetIFrameHandler(std::bind(&Master::DefaultRecviverHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        frame_.SetUFrameHandler(std::bind(&Master::ConnectionHandler, this, std::placeholders::_1));

        buffer_.reserve(frame_limit);
        work_ = std::thread(&Master::MainThread, this);
    }
}

void Master::Stop() {
    if (running_) {
        running_ = false;
    }

    if (work_.joinable()) {
        work_.join();
    }

    buffer_.clear();
    buffer_.shrink_to_fit();
}

void Master::MainThread() {
    running_ = true;
    while (running_) {
        if (!frame_.Run()) {
            if (connection_ev_handler_) {
                running_ = connection_ev_handler_(CONNECTION_BROKEN);
            } else {
                running_ = false;
            }
        }
    }
}

}  // namespace protocol