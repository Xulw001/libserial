#ifndef _SERIAL_BASE_H
#define _SERIAL_BASE_H
#include <stdint.h>

#include <string>

namespace raw {

enum SerialPortError {
    SERIAL_PORT_ERROR_NONE = 0,
    SERIAL_PORT_ERROR_INVALID_ARGUMENT = 1,
    SERIAL_PORT_ERROR_INVALID_BAUDRATE = 2,
    SERIAL_PORT_ERROR_OPEN_FAILED = 3,
    SERIAL_PORT_ERROR_IO_FAILED = 4,
    SERIAL_PORT_ERROR_UNKNOWN = 99
};

class SerialPortBase {
   public:
    /// @brief  Create a new SerialPort instance
    /// @param interface_name identifier or name of the serial interface (e.g. "/dev/ttyS1" or "COM4")
    /// @param baud_rate the baud rate in baud (e.g. 9600)
    /// @param data_bits the number of data bits (usually 8)
    /// @param parity defines what kind of parity to use ('E' - even parity, 'O' - odd parity, 'N' - no parity)
    /// @param stop_bits the number of stop buts (usually 1)
    SerialPortBase(const char* interface_name, int baud_rate,
                   uint8_t data_bits,
                   char parity,
                   uint8_t stop_bits) : interface_name_(interface_name),
                                        baud_rate_(baud_rate),
                                        data_bits_(data_bits),
                                        parity_(parity),
                                        stop_bits_(stop_bits) {
        last_error_ = SERIAL_PORT_ERROR_NONE;
        is_open_ = false;
    }

    /// @brief Destroy the SerialPort instance and release all resources
    virtual ~SerialPortBase() { ; }

    /// @brief Open the serial interface
    /// @return true in case of success, false otherwise (use \ref SerialPort_getLastError for a detailed error code)
    virtual bool Open() = 0;

    /// @brief Close (release) the serial interface
    virtual void Close() = 0;

    /// @brief Discard all data in the input buffer of the serial interface
    virtual void Discard() = 0;

    /// @brief Read a byte from the interface
    /// @return value of read byte, or -1 in case of an error
    virtual int ReadByte() = 0;

    /// @brief Write the number of bytes from the buffer to the serial interface
    /// @param buffer the buffer containing the data to write
    /// @param length number of bytes to write
    /// @return number of bytes written, or -1 in case of an error
    virtual int Write(uint8_t* buffer, int length) = 0;

    /// @brief Set the timeout used for message reception
    /// @param timeout the timeout value in ms.
    virtual void SetTimeout(int timeout) = 0;

    /// @brief Get the error code of the last operation
    /// @return
    SerialPortError GetLastError() { return last_error_; }

    /// @brief Get the status of the serial interface
    /// @return
    bool is_open() { return is_open_; }

   protected:
    std::string interface_name_;
    int baud_rate_;
    uint8_t data_bits_;
    char parity_;
    uint8_t stop_bits_;
    SerialPortError last_error_;

    bool is_open_;
};

};  // namespace raw
#endif