#pragma once

#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include "ipc_protocol.h"
#include "input_state.h"

namespace emu {

class IPCServer {
public:
    IPCServer();
    ~IPCServer();

    bool Start();
    void Stop();

    bool IsClientConnected() const { return client_connected_.load(); }

    // Get latest input state (thread-safe)
    bool GetInputState(ControllerInputState& left, ControllerInputState& right);

private:
    void ServerThread();
    void ProcessMessage(const uint8_t* data, size_t length);

    HANDLE pipe_handle_ = INVALID_HANDLE_VALUE;
    std::atomic<bool> running_{false};
    std::atomic<bool> client_connected_{false};
    std::thread server_thread_;

    std::mutex state_mutex_;
    ControllerInputState left_state_;
    ControllerInputState right_state_;
    bool state_updated_ = false;
};

} // namespace emu
