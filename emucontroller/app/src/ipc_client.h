#pragma once

#include <windows.h>
#include <atomic>
#include <thread>
#include "ipc_protocol.h"
#include "input_state.h"

namespace emu {

class IPCClient {
public:
    IPCClient();
    ~IPCClient();

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return connected_.load(); }

    bool SendInputState(const ControllerInputState& left, const ControllerInputState& right);

    // Start/stop background connection thread
    void StartConnectionThread();
    void StopConnectionThread();

private:
    void ConnectionThread();

    HANDLE pipe_handle_ = INVALID_HANDLE_VALUE;
    std::atomic<bool> connected_{false};
    std::atomic<bool> running_{false};
    std::thread connection_thread_;
};

} // namespace emu
