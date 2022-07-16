#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <functional>

int64_t time_function(std::function<void(void)> func);

#endif