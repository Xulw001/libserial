#ifndef _LOG_H
#define _LOG_H
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace clog {

enum LEVLE {
    Debug,
    Info,
    Warning,
    Error,
};

class Logger {
   public:
    void init_logger(LEVLE level = Info, std::string filepath = "");

    ~Logger();
    Logger &operator()(LEVLE level);
    Logger &operator<<(const std::wstring &data);
    Logger &operator<<(const wchar_t *data);

    template <typename T>
    Logger &operator<<(const T data) {
        if (flag) {
            (*ofstream_) << data;
        }
        return *this;
    }

   private:
    LEVLE level_ = Info;
    std::ostream *ofstream_ = nullptr;
    bool stdout_ = false;
    bool flag = false;
};

extern Logger g_logger_;

};  // namespace clog

#define qDebug                   \
    clog::g_logger_(clog::Debug) \
        << "[" << __FUNCTION__ << "][" << __FILE__ << ":" << __LINE__ << "]"
#define qInfo                   \
    clog::g_logger_(clog::Info) \
        << "[" << __FUNCTION__ << "][" << __FILE__ << ":" << __LINE__ << "]"
#define qWarning                   \
    clog::g_logger_(clog::Warning) \
        << "[" << __FUNCTION__ << "][" << __FILE__ << ":" << __LINE__ << "]"
#define qError                   \
    clog::g_logger_(clog::Error) \
        << "[" << __FUNCTION__ << "][" << __FILE__ << ":" << __LINE__ << "]"

#endif