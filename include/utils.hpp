#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <functional>
#include <chrono>

template<typename T = std::chrono::milliseconds>
int64_t time_function(std::function<void(void)> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<T>(end - start).count();
}

#endif