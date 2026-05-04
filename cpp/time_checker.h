#ifndef TIME_CHECKER_H
#define TIME_CHECKER_H

#include <chrono>

class TimeChecker {
public:
    TimeChecker(double time_limit_secs = 0.0)
        : start_(std::chrono::high_resolution_clock::now()),
          time_limit_(time_limit_secs)
    {}

    bool time_exceeded() const {
        if (time_limit_ <= 0.0) return false;
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_;
        return elapsed.count() >= time_limit_;
    }

    double elapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_;
        return elapsed.count();
    }

    double remaining() const {
        return time_limit_ - elapsed();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
    double time_limit_;
};

#endif
