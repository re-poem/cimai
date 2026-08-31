#pragma once 

#ifdef _WIN32
#ifdef CIMAI_BUILD
#define CIMAI_API __declspec(dllexport)
#else
#define CIMAI_API __declspec(dllimport)
#endif
#else
#define CIMAI_API
#endif