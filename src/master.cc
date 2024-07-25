#include "master.h"

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
}

void Master::SetRecviverhandler(MessageReceivedHandler serial_receiver, void* param) {
    serial_receiver_ = serial_receiver;
    auto callback = std::bind(&Master::DefaultRecviverHandler, this, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4);
    frame_.SetRecviverhandler(callback, param);
}

bool Master::DefaultRecviverHandler(void* parameter, uint8_t* msg, int size, bool more) {
    buffer_.insert(buffer_.end(), msg, msg + size);
    if (!more) {
        serial_receiver_(parameter, buffer_.data(), (int)buffer_.size());
        buffer_.clear();
    }
    return true;
}

void Master::Start() {
    if (!running_) {
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
            running_ = false;
        }
    }
}

}  // namespace protocol