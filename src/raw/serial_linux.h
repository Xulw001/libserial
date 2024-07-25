#ifndef _SERIAL_LINUX_H
#define _SERIAL_LINUX_H
#ifdef __linux__

#include <time.h>

#include <string>

#include "serial_base.h"

namespace raw {

class SerialPortLinux : public SerialPortBase {
   public:
    SerialPortLinux(const char* interface_name, int baud_rate,
                    uint8_t data_bits,
                    char parity,
                    uint8_t stop_bits) : SerialPortBase(interface_name, baud_rate, data_bits, parity, stop_bits) {
        serial_fd_ = -1;
        read_timeout_.tv_sec = 0;
        read_timeout_.tv_usec = 100000; /* 100 ms */
    }

    virtual ~SerialPortLinux() {
        if (is_open_) Close();
    }

    virtual bool Open();

    virtual void Close();

    virtual void Discard();

    virtual int ReadByte();

    virtual int Write(uint8_t* buffer, int length);

    virtual void SetTimeout(int timeout);

   private:
    int serial_fd_;
    struct timeval read_timeout_;
};

}  // namespace raw

typedef raw::SerialPortLinux SerialPortCommon;

#endif
#endif