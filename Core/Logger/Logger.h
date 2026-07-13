#pragma once
#include <windows.h>

class Logger {
public:
    static void Initialize();
    static void Log(const char* msg);
    static void Shutdown();

private:
    static HANDLE g_hLog;
};