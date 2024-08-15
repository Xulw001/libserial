#include "serial_linux.h"
#ifdef __linux__
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace raw {

bool SerialPortLinux::Open() {
    serial_fd_ = open(interface_name_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
    if (serial_fd_ == -1) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return false;
    }

    struct termios tios;
    speed_t baudrate;

    tcgetattr(serial_fd_, &tios);

    switch (baud_rate_) {
        case 110:
            baudrate = B110;
            break;
        case 300:
            baudrate = B300;
            break;
        case 600:
            baudrate = B600;
            break;
        case 1200:
            baudrate = B1200;
            break;
        case 2400:
            baudrate = B2400;
            break;
        case 4800:
            baudrate = B4800;
            break;
        case 9600:
            baudrate = B9600;
            break;
        case 19200:
            baudrate = B19200;
            break;
        case 38400:
            baudrate = B38400;
            break;
        case 57600:
            baudrate = B57600;
            break;
        case 115200:
            baudrate = B115200;
            break;
        default:
            baudrate = B9600;
            last_error_ = SERIAL_PORT_ERROR_INVALID_BAUDRATE;
    }

    /* Set baud rate */
    if ((cfsetispeed(&tios, baudrate) < 0) || (cfsetospeed(&tios, baudrate) < 0)) {
        close(serial_fd_);
        serial_fd_ = -1;
        last_error_ = SERIAL_PORT_ERROR_INVALID_BAUDRATE;
        return false;
    }

    tios.c_cflag |= (CREAD | CLOCAL);

    /* Set data bits (5/6/7/8) */
    tios.c_cflag &= ~CSIZE;
    switch (data_bits_) {
        case 5:
            tios.c_cflag |= CS5;
            break;
        case 6:
            tios.c_cflag |= CS6;
            break;
        case 7:
            tios.c_cflag |= CS7;
            break;
        case 8:
        default:
            tios.c_cflag |= CS8;
            break;
    }

    /* Set stop bits (1/2) */
    if (stop_bits_ == 1)
        tios.c_cflag &= ~CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    if (parity_ == 'E') {
        tios.c_cflag |= PARENB;
        tios.c_cflag &= ~PARODD;
    } else if (parity_ == 'O') {
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    } else { /* 'N' */
        tios.c_cflag &= ~PARENB;
    }

    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    if (parity_ == 'N') {
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    tios.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    tios.c_iflag |= IGNBRK; /* Set ignore break to allow 0xff characters */
    tios.c_iflag |= IGNPAR;
    tios.c_oflag &= ~OPOST;

    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(serial_fd_, TCSANOW, &tios) < 0) {
        close(serial_fd_);
        serial_fd_ = -1;
        last_error_ = SERIAL_PORT_ERROR_INVALID_ARGUMENT;

        return false;
    }

    return is_open_ = true;
}

void SerialPortLinux::Close() {
    if (serial_fd_ != -1) {
        close(serial_fd_);
        serial_fd_ = -1;
    }
    is_open_ = false;
}

void SerialPortLinux::Discard() {
    tcflush(serial_fd_, TCIOFLUSH);
}

int SerialPortLinux::ReadByte() {
    last_error_ = SERIAL_PORT_ERROR_NONE;
    if (!is_open_) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return -1;
    }

    fd_set set;
    FD_ZERO(&set);
    FD_SET(serial_fd_, &set);
    int ret = select(serial_fd_ + 1, &set, NULL, NULL, &read_timeout_);
    if (ret == -1) {
        last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        return -1;
    } else if (ret == 0) {
        return -1;
    }

    uint8_t buf[1];
    read(serial_fd_, (char*)buf, 1);
    return (int)buf[0];
}

int SerialPortLinux::Write(uint8_t* buffer, int length) {
    last_error_ = SERIAL_PORT_ERROR_NONE;

    if (!is_open_) {
        last_error_ = SERIAL_PORT_ERROR_OPEN_FAILED;
        return -1;
    }

    ssize_t result = write(serial_fd_, buffer, length);
    if (result < 0) {
        if (errno == EIO) {
            last_error_ = SERIAL_PORT_ERROR_IO_FAILED;
        } else {
            last_error_ = SERIAL_PORT_ERROR_UNKNOWN;
        }
        return -1;
    }

    // tcdrain(serial_fd_);

    return result;
}

void SerialPortLinux::SetTimeout(int timeout) {
    read_timeout_.tv_sec = timeout / 1000;
    read_timeout_.tv_usec = (timeout % 1000) * 1000;
}

}  // namespace raw
#endif