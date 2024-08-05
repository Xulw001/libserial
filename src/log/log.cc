#include "log.h"

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <codecvt>
#endif

namespace clog {

Logger g_logger_;

std::string convert(const std::wstring &src) {
#ifdef _WIN32
    std::string dst;
    if (src.empty()) {
        return dst;
    }

    int size = -1;
    size = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) {
        return false;
    }

    dst.resize(size);
    size = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, (char *)dst.c_str(),
                               size, NULL, NULL);
    if (size <= 0) {
        return false;
    }

    dst.resize(size - 1);
    return dst;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t> > str_conv;
    return str_conv.to_bytes(src);
#endif
}

Logger &Logger::operator<<(const wchar_t *data) {
    if (flag) {
        (*ofstream_) << convert(data);
    }
    return *this;
}

Logger &Logger::operator<<(const std::wstring &data) {
    if (flag) {
        (*ofstream_) << convert(data);
    }
    return *this;
}

Logger &Logger::operator()(LEVLE level) {
    if (level >= level_) {
        auto t =
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        bool skip = false;
        if (ofstream_ == nullptr) {
            ofstream_ = &std::cout;
            skip = true;
        }

        if (ofstream_->good()) {
            tm tm;
            localtime_s(&tm, &t);
            if (!skip) {
                (*ofstream_) << std::endl;
            }

            (*ofstream_) << std::put_time(&tm, "%Y:%m:%d %H:%M:%S ");
            switch (level) {
                case Debug:
                    (*ofstream_) << "[Debug]";
                    break;
                case Warning:
                    (*ofstream_) << "[Warning]";
                    break;
                case Error:
                    (*ofstream_) << "[Error]";
                    break;
                case Info:
                default:
                    (*ofstream_) << "[Info]";
                    break;
            }
        }
        flag = true;
    } else {
        flag = false;
    }

    return *this;
}

};  // namespace clog