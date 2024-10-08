#include "frame.h"

#include <string.h>
#ifdef __linux__
#include <sys/time.h>
#else
#include <Windows.h>
#endif

#include "crc/crc.h"
#include "endian.h"
#include "log/log.h"

namespace protocol {

enum MsgState { STATE_IDLE,
                STATE_SENDED,
                STATE_SEND_CONGIRMED,
};

#define MAX_SIZE 0x4000
struct sMsg {
    MsgState state;
    uint64_t send_time;
    int size;
    uint8_t data[MAX_SIZE];
};

#define FIXED_MSG_SIZE 4
static uint8_t STARTDT_ACT_MSG[] = {cUmark, START, 0x7, cEmark};
static uint8_t STARTDT_CON_MSG[] = {cUmark, STARTC, 0xe, cEmark};
static uint8_t RESETDT_ACT_MSG[] = {cUmark, RESET, 0x1c, cEmark};
static uint8_t RESETDT_CON_MSG[] = {cUmark, RESETC, 0x38, cEmark};
static uint8_t STOPDT_ACT_MSG[] = {cUmark, STOP, 0x70, cEmark};
static uint8_t STOPDT_CON_MSG[] = {cUmark, STOPC, 0xe0, cEmark};
static uint8_t TESTFR_ACT_MSG[] = {cUmark, TESTFR, 0xc7, cEmark};
static uint8_t TESTFR_CON_MSG[] = {cUmark, TESTFRC, 0x89, cEmark};

APCIParameters default_apci_parameters = {
    /* .time_alive = */ 15,
    /* .time_heart = */ 20};

Frame::Frame(SerialPortBase* serial_connection) : Frame(serial_connection, default_apci_parameters) {}

Frame::Frame(SerialPortBase* serial_connection, const APCIParameters apci_parameters)
    : frame_handler_(serial_connection), apci_parameters_(apci_parameters) {
    ResetAll();
}

Frame::~Frame() { msg_queue_.clear(); }

void Frame::MessageHandler(void* parameter, uint8_t* msg, int size) {
    int crc_flg = 0;
    uint8_t* content = 0;
    int len = 0;
    if (msg[0] == cImark) {
        qDebug << "recv I frame!";
        if (*(uint16_t*)&msg[1] != *(uint16_t*)&msg[3]) {
            qWarning << "frame size miss!";
            return;
        }

        /* check if message size is reasonable */
        uint16_t msg_size = (cint16(msg[1], msg[2])) & 0x7fff;
        if (size != msg_size + cIFixedLength) {
            qWarning << "frame size miss!";
            return;
        }

        content = msg + cIHeaderLength;
        len = msg_size + 2;
        crc_flg = 16;
    } else if (msg[0] == cUmark) {
        qDebug << "recv U frame!";
        content = msg + 1;
        len = 1;
        crc_flg = 8;
    } else if (msg[0] == cAmark) {
        qDebug << "recv Ack frame!";
        crc_flg = 0;
    } else {
        return;
    }

    /* check checksum */
    switch (crc_flg) {
        case 8: {
            uint8_t checksum = crc::crc8(content, len);
            if (checksum != msg[size - 2]) {
                qWarning << "frame checksum error!";
                return;
            }

            /* U-frame ACK */
        } break;
        case 16: {
            uint16_t checksum = crc::crc16(content, len);
            if (checksum != cint16(msg[size - 3], msg[size - 2])) {
                qWarning << "frame checksum error!";
                return;
            }

        } break;
        default:
            break;
    }
}

bool Frame::Run() {
    uint8_t buffer[MAX_SIZE] = {0};
    bool alive = frame_handler_.ReadNextMessage(buffer, Frame::MessageHandler, nullptr);
    switch (buffer[0]) {
        /* handle u-frame */
        case cUmark:
            switch (buffer[1]) {
                case START:
                    frame_handler_.SendSingleMessage(STARTDT_CON_MSG, FIXED_MSG_SIZE);
                    qDebug << "confirmed start frame!";
                    break;
                case RESET:
                    frame_handler_.SendSingleMessage(RESETDT_CON_MSG, FIXED_MSG_SIZE);
                    recv_frame_no_ = 0;
                    qDebug << "confirmed reset frame!";
                    break;
                case STOP:
                    frame_handler_.SendSingleMessage(STOPDT_CON_MSG, FIXED_MSG_SIZE);
                    qDebug << "confirmed stop frame!";
                    break;
                case TESTFR:
                    frame_handler_.SendSingleMessage(TESTFR_CON_MSG, FIXED_MSG_SIZE);
                    qDebug << "confirmed test frame!";
                    break;
                case STOPC:
                case STARTC:
                case RESETC: {
                    qDebug << "recv U confirmed frame!";
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    auto it = msg_queue_.begin();
                    if (it->data[1] == (buffer[1] >> 1)) {
                        msg_queue_.erase(msg_queue_.begin());
                    }
                } break;
                case TESTFRC:
                    qDebug << "recv test confirmed frame!";
                    break;
                default:
                    break;
            }
            if (u_handler_) {
                u_handler_((UFrame)buffer[1]);
            }

            break;
        /* handle i-frame */
        case cImark: {
            uint16_t recv_frame_no = *(uint16_t*)(buffer + cIHeaderLength);
            if (recv_frame_no_ > recv_frame_no) {
                qError << "frame number error!";
                ResetAll();
                return false;
            } else if (recv_frame_no_ < recv_frame_no) {
                if (i_handler_) {
                    uint16_t msg_size = cint16(buffer[1], buffer[2]);
                    i_handler_(buffer + cIDataOffset, msg_size & 0x7fff,
                               msg_size >> 0xF);
                }
                recv_frame_no_ = recv_frame_no % 0xffff;
            }

            if (!frame_handler_.SendSingleMessage((uint8_t*)&cAmark, 1)) {
                ResetAll();
                return false;
            }
            qDebug << "send Ack frame at " << recv_frame_no_;
        } break;
        /* handle ack */
        case cAmark: {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (msg_queue_.size() && msg_queue_.begin()->state == STATE_SENDED) {
                msg_queue_.erase(msg_queue_.begin());
                if (send_frame_no_ >= 0xffff) {
                    send_frame_no_ = 0;
                }
                send_frame_no_++;
            }
        } break;
        default:
            break;
    }

    // when receive any frame, reset next heart time
    if (alive) {
        ResetTimeout();
        no_confirm_msg_ = 0;
    }

    if (msg_queue_.size()) {
        if (!SendSingleMessage()) {
            ResetAll();
            return false;
        }
    }

    if (!HandleTimeout()) {
        ResetAll();
        return false;
    }

    return true;
}

void Frame::ResetAll() {
    ResetTimeout();
    no_confirm_msg_ = 0;
    send_frame_no_ = 1;
    recv_frame_no_ = 0;
    msg_queue_.clear();
}

static uint64_t Hal_getTimeInMs() {
#ifdef __linux__
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((uint64_t)now.tv_sec * 1000LL) + (now.tv_usec / 1000);
#else
    FILETIME ft;
    uint64_t now;
    static const uint64_t DIFF_TO_UNIXTIME = 11644473600000ULL;
    GetSystemTimeAsFileTime(&ft);
    now = (LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL);
    return (now / 10000LL) - DIFF_TO_UNIXTIME;
#endif
}

void Frame::ResetTimeout() {
    next_heart_timeout_ = Hal_getTimeInMs() + (uint64_t)(apci_parameters_.time_heart * 1000);
}

bool Frame::HandleTimeout() {
    uint64_t currentTime = Hal_getTimeInMs();
    if (currentTime > next_heart_timeout_) {
        if (no_confirm_msg_ > 2) {  // testfr frame not confirm
            qError << "heart timeout overflow!";
            return false;
        } else {  // send frame to testfr
            qDebug << "send heart again!";
            if (!frame_handler_.SendSingleMessage(TESTFR_ACT_MSG, FIXED_MSG_SIZE))
                return false;
            ResetTimeout();
            no_confirm_msg_++;
        }
    }

    if (msg_queue_.size()) {
        std::lock_guard<std::mutex> lock_queue(queue_mutex_);
        auto it = msg_queue_.begin();
        if (it->state == STATE_SENDED && currentTime > it->send_time) {
            // data frame not confirm along with alive time
            if (currentTime - it->send_time >= (uint64_t)(apci_parameters_.time_alive * 1000)) {
                if (!frame_handler_.SendSingleMessage(it->data, it->size))
                    return false;
                qWarning << "i frame send unconfirmed!";
                it->send_time = currentTime;
                return true;
            }
        }
    }

    return true;
}

void Frame::SendFrame(uint8_t* data, int size) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    sMsg frame = {STATE_IDLE, 0, size, 0};
    memcpy(frame.data, data, size);
    msg_queue_.emplace_back(frame);
}

bool Frame::SendSingleMessage() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto it = msg_queue_.begin();
    if (it->state == STATE_IDLE) {
        if (it->data[0] == cImark) {
            uint8_t* frame_data = it->data;
            memcpy(frame_data + cIHeaderLength, &send_frame_no_, sizeof(uint16_t));
            uint16_t crc_size = it->size - cIFixedLength + 2;
            uint16_t check_sum = crc::crc16(frame_data + cIHeaderLength, crc_size);
            memcpy(frame_data + cIHeaderLength + crc_size, &check_sum, sizeof(uint16_t));
            qDebug << "send I frame at " << send_frame_no_;
        } else {
            qDebug << "send U frame!";
        }

        if (!frame_handler_.SendSingleMessage(it->data, it->size)) {
            return false;
        }
        it->state = STATE_SENDED;
        it->send_time = Hal_getTimeInMs();
    }
    return true;
}

int Frame::PrepareUFrame(UFrame type, uint8_t* frame_data) {
    int len = FIXED_MSG_SIZE;
    switch (type) {
        case START:
            memcpy(frame_data, STARTDT_ACT_MSG, FIXED_MSG_SIZE);
            break;
        case STARTC:
            memcpy(frame_data, STARTDT_CON_MSG, FIXED_MSG_SIZE);
            break;
        case RESET:
            memcpy(frame_data, RESETDT_ACT_MSG, FIXED_MSG_SIZE);
            break;
        case RESETC:
            memcpy(frame_data, RESETDT_CON_MSG, FIXED_MSG_SIZE);
            break;
        case STOP:
            memcpy(frame_data, STOPDT_ACT_MSG, FIXED_MSG_SIZE);
            break;
        case STOPC:
            memcpy(frame_data, STOPDT_CON_MSG, FIXED_MSG_SIZE);
            break;
        case TESTFR:
            memcpy(frame_data, TESTFR_ACT_MSG, FIXED_MSG_SIZE);
            break;
        case TESTFRC:
            memcpy(frame_data, TESTFR_CON_MSG, FIXED_MSG_SIZE);
            break;
        default:
            len = 0;
            break;
    }
    return len;
}

int Frame::PrepareIFrame(uint8_t* data, int size, uint8_t* frame_data, bool more) {
    if (data != NULL && size != 0) {
        frame_data[0] = cImark;
        // TODO(endian)
        uint16_t mark_size = more ? (size | 0x8000) : (size & 0x7fff);
        memcpy(frame_data + 1, &mark_size, sizeof(uint16_t));
        memcpy(frame_data + 3, &mark_size, sizeof(uint16_t));
        frame_data[5] = cImark;
        memcpy(frame_data + cIDataOffset, data, size);
        frame_data[size + cIFixedLength - 1] = 0x10;
        return size + cIFixedLength;
    }
    return 0;
}
};  // namespace protocol