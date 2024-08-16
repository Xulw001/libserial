#include "serial_win32.h"
#ifdef _WIN32

namespace raw {

bool SerialPortWin::Open() {
    COMMTIMEOUTS timeouts = {0};
    serial_handle_ = CreateFileA(interface_name_.c_str(), GENERIC_READ | GENERIC_WRITE,
                                 0, NULL, OPEN_EXISTING, 0, NULL);
    if (serial_handle_ == INVALID_HANDLE_VALUE) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return false;
    }

    DCB serial_params = {0};
    serial_params.DCBlength = sizeof(DCB);
    LPDCB serialParams = &serial_params;
    BOOL status = GetCommState(serial_handle_, serialParams);
    if (status == false) {
        last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        goto exit_error;
    }

    /* set baud rate */
    switch (baud_rate_) {
        case 110:
            serialParams->BaudRate = CBR_110;
            break;
        case 300:
            serialParams->BaudRate = CBR_300;
            break;
        case 600:
            serialParams->BaudRate = CBR_600;
            break;
        case 1200:
            serialParams->BaudRate = CBR_1200;
            break;
        case 2400:
            serialParams->BaudRate = CBR_2400;
            break;
        case 4800:
            serialParams->BaudRate = CBR_4800;
            break;
        case 9600:
            serialParams->BaudRate = CBR_9600;
            break;
        case 19200:
            serialParams->BaudRate = CBR_19200;
            break;
        case 38400:
            serialParams->BaudRate = CBR_38400;
            break;
        case 57600:
            serialParams->BaudRate = CBR_57600;
            break;
        case 115200:
            serialParams->BaudRate = CBR_115200;
            break;
        default:
            serialParams->BaudRate = CBR_9600;
            last_error_ = SERIAL_PORT_ERROR_INVALID_BAUDRATE;
    }

    /* Set data bits (5/6/7/8) */
    serialParams->ByteSize = data_bits_;

    /* Set stop bits (1/2) */
    if (stop_bits_ == 1)
        serialParams->StopBits = ONESTOPBIT;
    else /* 2 */
        serialParams->StopBits = TWOSTOPBITS;

    if (parity_ == 'N')
        serialParams->Parity = NOPARITY;
    else if (parity_ == 'E')
        serialParams->Parity = EVENPARITY;
    else /* 'O' */
        serialParams->Parity = ODDPARITY;

    status = SetCommState(serial_handle_, serialParams);
    if (status == false) {
        last_error_ = SERIAL_PORT_ERROR_INVALID_ARGUMENT;
        goto exit_error;
    }

    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    status = SetCommTimeouts(serial_handle_, &timeouts);
    if (status == false) {
        last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        goto exit_error;
    }

    status = SetCommMask(serial_handle_, EV_RXCHAR);
    if (status == false) {
        last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        goto exit_error;
    }

    last_error_ = SERIAL_PORT_ERROR_NONE;
    return is_open_ = true;

exit_error:

    if (serial_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_handle_);
        serial_handle_ = INVALID_HANDLE_VALUE;
    }

    return false;
}

void SerialPortWin::Close() {
    if (serial_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(serial_handle_);
        serial_handle_ = INVALID_HANDLE_VALUE;
    }
    is_open_ = false;
}

void SerialPortWin::Discard() {
    PurgeComm(serial_handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

int SerialPortWin::ReadByte() {
    last_error_ = SERIAL_PORT_ERROR_NONE;
    if (!is_open_) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return -1;
    }

    uint8_t buf[1];
    DWORD bytesRead = 0;
    BOOL status = ReadFile(serial_handle_, buf, 1, &bytesRead, NULL);
    if (status == false) {
        last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        return -1;
    }

    last_error_ = SERIAL_PORT_ERROR_NONE;
    if (bytesRead == 0)
        return -1;
    else
        return (int)buf[0];
}

int SerialPortWin::Write(uint8_t* buffer, int length) {
    last_error_ = SERIAL_PORT_ERROR_NONE;

    if (!is_open_) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return -1;
    }

    DWORD numberOfBytesWritten = 0;
    BOOL status = WriteFile(serial_handle_, buffer, length, &numberOfBytesWritten, NULL);
    if (status == false) {
        switch (::GetLastError()) {
            case ERROR_BAD_COMMAND:
                last_error_ = SERIAL_PORT_ERROR_IO_FAILED;
                break;
            case ERROR_SEM_TIMEOUT:
                break;
            default:
                last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
                break;
        }
        return -1;
    }

    status = FlushFileBuffers(serial_handle_);

    if (status == false) {
        ;
    }

    return (int)numberOfBytesWritten;
}

void SerialPortWin::SetTimeout(int timeout) {
    read_timeout_ = timeout;
}

}  // namespace raw
#endif