#pragma once 

#ifdef _WIN32
#define CIMAI_API __declspec(dllexport)
#else
#define CIMAI_API
#endif