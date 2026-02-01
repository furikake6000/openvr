#pragma once

#include <openvr_driver.h>
#include <string>

extern void DriverLog(const char* pFormat, ...);
extern bool InitDriverLog(vr::IVRDriverLog* pDriverLog);
extern void CleanupDriverLog();
