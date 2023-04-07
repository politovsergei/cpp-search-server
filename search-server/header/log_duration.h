#pragma once

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

#include <iostream>
#include <chrono>

using namespace std::literals;

class LogDuration {
    public:
        explicit LogDuration();
        explicit LogDuration(const std::string name);
        ~LogDuration();

    private:
        const std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
        const std::string call_name;
};

LogDuration::LogDuration() {}

LogDuration::LogDuration(const std::string name) : call_name(name) {};

LogDuration::~LogDuration() {
    std::cerr << call_name << ": "s << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time_).count() << " ms"s << std::endl;
}