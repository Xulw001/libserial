#ifndef _SERIAL_WIN32_H
#define _SERIAL_WIN32_H
#ifdef _WIN32
#include <Windows.h>

#include "serial_base.h"

namespace raw {

class SerialPortWin : public SerialPortBase {
   public:
    SerialPortWin(const char* interface_name, int baud_rate, uint8_t data_bits, char parity, uint8_t stop_bits)
        : SerialPortBase(interface_name, baud_rate, data_bits, parity, stop_bits) {
        serial_handle_ = INVALID_HANDLE_VALUE;
        read_timeout_ = 100;
    }

    virtual ~SerialPortWin() {
        if (is_open_) Close();
    }

    virtual bool Open();

    virtual void Close();

    virtual void Discard();

    virtual int ReadByte();

    virtual int Write(uint8_t* buffer, int length);

    virtual void SetTimeout(int timeout);

   private:
    HANDLE serial_handle_;
    int read_timeout_;
};

}  // namespace raw

typedef raw::SerialPortWin SerialPortCommon;

#endif
#endif