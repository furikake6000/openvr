#include "ipc_client.h"
#include <chrono>

namespace emu {

IPCClient::IPCClient() = default;

IPCClient::~IPCClient() {
    StopConnectionThread();
    Disconnect();
}

bool IPCClient::Connect() {
    if (connected_.load()) return true;

    // Try to connect to the named pipe
    pipe_handle_ = CreateFileA(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Set pipe to message mode
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(pipe_handle_, &mode, nullptr, nullptr)) {
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    // Send connect message
    MessageHeader connect_msg;
    connect_msg.type = MSG_CONNECT;
    connect_msg.length = 0;

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe_handle_, &connect_msg, sizeof(connect_msg), &bytesWritten, nullptr)) {
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    connected_ = true;
    return true;
}

void IPCClient::Disconnect() {
    if (!connected_.load()) return;

    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        // Send disconnect message
        MessageHeader disconnect_msg;
        disconnect_msg.type = MSG_DISCONNECT;
        disconnect_msg.length = 0;

        DWORD bytesWritten = 0;
        WriteFile(pipe_handle_, &disconnect_msg, sizeof(disconnect_msg), &bytesWritten, nullptr);

        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }

    connected_ = false;
}

bool IPCClient::SendInputState(const ControllerInputState& left, const ControllerInputState& right) {
    if (!connected_.load() || pipe_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    InputStateMessage msg;
    msg.header.type = MSG_INPUT_STATE;
    msg.header.length = sizeof(InputStateMessage) - sizeof(MessageHeader);

    // Convert left controller
    msg.left.buttons = left.buttons;
    msg.left.stick_x = FloatToStick(left.stick_x);
    msg.left.stick_y = FloatToStick(left.stick_y);
    msg.left.trigger = FloatToTrigger(left.trigger);
    msg.left.grip = FloatToTrigger(left.grip);

    // Convert right controller
    msg.right.buttons = right.buttons;
    msg.right.stick_x = FloatToStick(right.stick_x);
    msg.right.stick_y = FloatToStick(right.stick_y);
    msg.right.trigger = FloatToTrigger(right.trigger);
    msg.right.grip = FloatToTrigger(right.grip);

    // Timestamp
    msg.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe_handle_, &msg, sizeof(msg), &bytesWritten, nullptr)) {
        // Connection lost
        connected_ = false;
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

void IPCClient::StartConnectionThread() {
    if (running_.load()) return;

    running_ = true;
    connection_thread_ = std::thread(&IPCClient::ConnectionThread, this);
}

void IPCClient::StopConnectionThread() {
    if (!running_.load()) return;

    running_ = false;
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }
}

void IPCClient::ConnectionThread() {
    while (running_.load()) {
        if (!connected_.load()) {
            Connect();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace emu
