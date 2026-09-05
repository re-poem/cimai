#pragma once 

#if defined(_WIN32)
#define CIMAI_API __declspec(dllexport)
#elif defined(__GNUC__)
#define CIMAI_API __attribute__((visibility("default")))
#else
#define CIMAI_API
#endif