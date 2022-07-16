#include "utils.hpp"

#include <chrono>
#include <functional>
#include <cstdint>

int64_t time_function(std::function<void(void)> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}