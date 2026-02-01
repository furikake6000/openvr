#include "ipc_server.h"
#include "driverlog.h"
#include <vector>

namespace emu {

IPCServer::IPCServer() = default;

IPCServer::~IPCServer() {
    Stop();
}

bool IPCServer::Start() {
    if (running_.load()) return false;

    running_ = true;
    server_thread_ = std::thread(&IPCServer::ServerThread, this);
    DriverLog("IPC Server started\n");
    return true;
}

void IPCServer::Stop() {
    if (!running_.load()) return;

    running_ = false;

    // Close pipe to unblock any waiting operations
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        CancelIoEx(pipe_handle_, nullptr);
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    client_connected_ = false;
    DriverLog("IPC Server stopped\n");
}

bool IPCServer::GetInputState(ControllerInputState& left, ControllerInputState& right) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!state_updated_) return false;

    left = left_state_;
    right = right_state_;
    state_updated_ = false;
    return true;
}

void IPCServer::ServerThread() {
    DriverLog("IPC Server thread running\n");

    while (running_.load()) {
        // Create named pipe
        pipe_handle_ = CreateNamedPipeA(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,          // Max instances
            1024,       // Out buffer size
            1024,       // In buffer size
            0,          // Default timeout
            nullptr     // Security attributes
        );

        if (pipe_handle_ == INVALID_HANDLE_VALUE) {
            DriverLog("Failed to create named pipe: %d\n", GetLastError());
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        DriverLog("Waiting for client connection...\n");

        // Wait for client connection
        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

        BOOL connected = ConnectNamedPipe(pipe_handle_, &overlapped);
        if (!connected) {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Wait for connection with timeout to check running_ flag
                while (running_.load()) {
                    DWORD result = WaitForSingleObject(overlapped.hEvent, 100);
                    if (result == WAIT_OBJECT_0) {
                        connected = TRUE;
                        break;
                    }
                }
            } else if (error == ERROR_PIPE_CONNECTED) {
                connected = TRUE;
            }
        }

        CloseHandle(overlapped.hEvent);

        if (!connected || !running_.load()) {
            CloseHandle(pipe_handle_);
            pipe_handle_ = INVALID_HANDLE_VALUE;
            continue;
        }

        client_connected_ = true;
        DriverLog("Client connected!\n");

        // Read loop
        std::vector<uint8_t> buffer(1024);
        while (running_.load() && client_connected_.load()) {
            DWORD bytesRead = 0;
            BOOL success = ReadFile(
                pipe_handle_,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                nullptr
            );

            if (!success || bytesRead == 0) {
                DWORD error = GetLastError();
                if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
                    DriverLog("Client disconnected\n");
                    client_connected_ = false;
                    break;
                }
                // Other error, continue
                continue;
            }

            ProcessMessage(buffer.data(), bytesRead);
        }

        // Cleanup
        DisconnectNamedPipe(pipe_handle_);
        CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
        client_connected_ = false;
    }

    DriverLog("IPC Server thread exiting\n");
}

void IPCServer::ProcessMessage(const uint8_t* data, size_t length) {
    if (length < sizeof(MessageHeader)) return;

    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);

    switch (header->type) {
        case MSG_INPUT_STATE: {
            if (length >= sizeof(InputStateMessage)) {
                const InputStateMessage* msg = reinterpret_cast<const InputStateMessage*>(data);

                std::lock_guard<std::mutex> lock(state_mutex_);

                // Convert left controller
                left_state_.buttons = msg->left.buttons;
                left_state_.stick_x = StickToFloat(msg->left.stick_x);
                left_state_.stick_y = StickToFloat(msg->left.stick_y);
                left_state_.trigger = TriggerToFloat(msg->left.trigger);
                left_state_.grip = TriggerToFloat(msg->left.grip);

                // Convert right controller
                right_state_.buttons = msg->right.buttons;
                right_state_.stick_x = StickToFloat(msg->right.stick_x);
                right_state_.stick_y = StickToFloat(msg->right.stick_y);
                right_state_.trigger = TriggerToFloat(msg->right.trigger);
                right_state_.grip = TriggerToFloat(msg->right.grip);

                state_updated_ = true;
            }
            break;
        }

        case MSG_CONNECT: {
            DriverLog("Received connect message from client\n");
            // Send status response
            StatusResponse response;
            response.header.type = MSG_STATUS_RESPONSE;
            response.header.length = sizeof(StatusResponse) - sizeof(MessageHeader);
            response.driver_active = 1;
            response.left_connected = 1;
            response.right_connected = 1;
            response.reserved = 0;

            DWORD bytesWritten = 0;
            WriteFile(pipe_handle_, &response, sizeof(response), &bytesWritten, nullptr);
            break;
        }

        case MSG_DISCONNECT: {
            DriverLog("Received disconnect message from client\n");
            client_connected_ = false;
            break;
        }

        default:
            DriverLog("Unknown message type: %d\n", header->type);
            break;
    }
}

} // namespace emu
