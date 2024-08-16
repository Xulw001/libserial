#include "log.h"

#include <fstream>
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

Logger::~Logger() {
    if (!stdout_) {
        if (ofstream_) {
            if (((std::ofstream *)ofstream_)->is_open()) {
                ((std::ofstream *)ofstream_)->close();
            }
            delete ofstream_;
        }
    }
}

void Logger::init_logger(LEVLE level, std::string filepath) {
    if (filepath.empty()) {
        ofstream_ = &std::cout;
        stdout_ = true;
    } else {
        ofstream_ = new std::ofstream(filepath, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
        stdout_ = false;
    }
    level_ = level;
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

        if (ofstream_->good()) {
            tm tm;
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            static bool skip = true;
            if (!skip) {
                (*ofstream_) << std::endl;
            } else {
                skip = false;
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