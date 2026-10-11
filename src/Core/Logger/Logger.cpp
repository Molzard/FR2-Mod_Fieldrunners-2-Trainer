#include "pch.h"
#include "Logger.h"

HANDLE Logger::g_hLog = INVALID_HANDLE_VALUE;

void Logger::Initialize() {
    g_hLog = CreateFileA("fr2_test_log.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void Logger::Log(const char* msg) {
    if (g_hLog == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(g_hLog, msg, (DWORD)strlen(msg), &written, NULL);
    WriteFile(g_hLog, "\r\n", 2, &written, NULL);
}

void Logger::Shutdown()
{
    if (g_hLog != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hLog);
        g_hLog = INVALID_HANDLE_VALUE;
    }
}