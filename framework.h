#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <wrl.h>
#include <unordered_set>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace std;
using namespace Microsoft::WRL;

template<class T> inline void SafeRelease(T** ppT) {
    if (*ppT != NULL)
    {
        (*ppT)->Release();
        (*ppT) = NULL;
    }
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

#define PI 3.14159265358979323846
#define SQRT_2 1.41421356237309504880

using namespace std;