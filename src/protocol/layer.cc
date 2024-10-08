#include "layer.h"

#include <memory>

#include "endian.h"

namespace protocol {

bool Layer::SendSingleMessage(uint8_t* msg, int size) {
    if (!serial_connection_->is_open() &&
        !serial_connection_->Open()) {
        return false;
    }

    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size + 2]);
    memcpy(buffer.get(), &cBmark, sizeof(cBmark));
    memcpy(buffer.get() + 1, msg, size);
    if (serial_connection_->Write(buffer.get(), size + 1) > 0) {
        return true;
    }

    return false;
}

int Layer::ReadBytesWithTimeout(uint8_t* buffer, int count) {
    int read;
    int bytes = 0;

    while ((read = serial_connection_->ReadByte()) != -1) {
        buffer[bytes++] = (uint8_t)read;
        if (bytes >= count)
            break;
    }

    return bytes;
}

bool Layer::ReadNextMessage(uint8_t* buffer, SerialMessageHandler message_handler, void* parameter) {
    if (!serial_connection_->is_open() &&
        !serial_connection_->Open()) {
        return false;
    }

    serial_connection_->SetTimeout(message_timeout_);
    do {
        int read = serial_connection_->ReadByte();
        if (read < 0)
            break;

        if (read != cBmark)
            continue;

        read = serial_connection_->ReadByte();
        if (read < 0)
            break;

        if (read == cImark) {
            serial_connection_->SetTimeout(character_timeout_);

            int l_size = serial_connection_->ReadByte();
            int h_size = serial_connection_->ReadByte();
            if (l_size == -1 || h_size == -1) {
                continue;
            }

            int msg_size = (cint16(l_size, h_size)) & 0x7fff;
            buffer[0] = cImark;
            buffer[1] = l_size;
            buffer[2] = h_size;
            msg_size += 3 + cIFixedLength - cIHeaderLength;

            int bytes = ReadBytesWithTimeout(buffer + 3, msg_size);
            if (bytes == msg_size) {
                msg_size += 3;
                message_handler(parameter, buffer, msg_size);
                return true;
            }

        } else if (read == cUmark) {
            serial_connection_->SetTimeout(character_timeout_);

            buffer[0] = cUmark;
            int msg_size = cUFixedLength - sizeof(cUmark);
            int bytes = ReadBytesWithTimeout(buffer + sizeof(cUmark), msg_size);
            if (bytes == msg_size) {
                msg_size += sizeof(cUmark);
                message_handler(parameter, buffer, msg_size);
                return true;
            }

        } else if (read == cAmark) {
            buffer[0] = cAmark;
            message_handler(parameter, buffer, (int)sizeof(cAmark));
            return true;
        }
    } while (true);

    return false;
}

}  // namespace protocol