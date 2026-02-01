#include "driverlog.h"
#include <cstdio>
#include <cstdarg>

static vr::IVRDriverLog* s_pLogFile = nullptr;

bool InitDriverLog(vr::IVRDriverLog* pDriverLog) {
    if (s_pLogFile) return false;
    s_pLogFile = pDriverLog;
    return s_pLogFile != nullptr;
}

void CleanupDriverLog() {
    s_pLogFile = nullptr;
}

void DriverLog(const char* pFormat, ...) {
    va_list args;
    va_start(args, pFormat);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), pFormat, args);

    if (s_pLogFile) {
        s_pLogFile->Log(buffer);
    }

    va_end(args);
}
