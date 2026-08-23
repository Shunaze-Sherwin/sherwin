#include "bot_process_win.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX  // MinGW đã tự định nghĩa sẵn trong os_defines.h
#define NOMINMAX
#endif
#include <windows.h>

#include <vector>

namespace winproc {

namespace {

bool file_exists(const std::string &path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::string resolve_bot_path(const char *bot) {
    std::string path = bot;
    bool has_exe = path.size() >= 4 &&
                   _stricmp(path.c_str() + path.size() - 4, ".exe") == 0;
    if (!has_exe && file_exists(path + ".exe")) return path + ".exe";
    return path;
}

}  // namespace

bool spawn(const char *bot, Handles &handles) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE to_bot_read = nullptr, to_bot_write = nullptr;
    HANDLE from_bot_read = nullptr, from_bot_write = nullptr;
    if (!CreatePipe(&to_bot_read, &to_bot_write, &attributes, 0)) return false;
    if (!CreatePipe(&from_bot_read, &from_bot_write, &attributes, 0)) {
        CloseHandle(to_bot_read);
        CloseHandle(to_bot_write);
        return false;
    }
    // Đầu ống phía CHA không được kế thừa, nếu không bot con sẽ giữ luôn đầu ghi
    // của chính ống nó đọc -> đóng ống không bao giờ sinh EOF, judge treo khi thoát.
    SetHandleInformation(to_bot_write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(from_bot_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = to_bot_read;
    startup.hStdOutput = from_bot_write;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::string command = "\"" + resolve_bot_path(bot) + "\" --interactive";
    std::vector<char> buffer(command.begin(), command.end());
    buffer.push_back('\0');

    PROCESS_INFORMATION information{};
    BOOL started = CreateProcessA(nullptr, buffer.data(), nullptr, nullptr, TRUE, 0,
                                  nullptr, nullptr, &startup, &information);

    CloseHandle(to_bot_read);
    CloseHandle(from_bot_write);
    if (!started) {
        CloseHandle(to_bot_write);
        CloseHandle(from_bot_read);
        return false;
    }
    CloseHandle(information.hThread);

    handles.process = information.hProcess;
    handles.input = to_bot_write;
    handles.output = from_bot_read;
    return true;
}

bool write_all(const Handles &handles, const std::string &data) {
    if (!handles.input) return false;
    size_t sent = 0;
    while (sent < data.size()) {
        DWORD written = 0;
        if (!WriteFile(handles.input, data.data() + sent,
                       static_cast<DWORD>(data.size() - sent), &written, nullptr) ||
            written == 0)
            return false;
        sent += written;
    }
    return true;
}

bool read_line(const Handles &handles, int timeout_ms, std::string &line) {
    line.clear();
    if (!handles.output) return false;
    ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(timeout_ms);
    while (true) {
        DWORD available = 0;
        // PeekNamedPipe thay cho poll(): ReadFile trên ống là chặn vô hạn, không có
        // cách đặt timeout trực tiếp, nên phải hỏi trước xem đã có byte nào chưa.
        if (!PeekNamedPipe(handles.output, nullptr, 0, nullptr, &available, nullptr))
            return false;
        if (available == 0) {
            if (GetTickCount64() >= deadline) return false;
            Sleep(1);
            continue;
        }
        char value = 0;
        DWORD count = 0;
        if (!ReadFile(handles.output, &value, 1, &count, nullptr) || count != 1)
            return false;
        if (value == '\n') return true;
        line.push_back(value);
    }
}

void destroy(Handles &handles) {
    // Đóng đầu ghi trước để bot thấy EOF và tự thoát (Sokoban còn lưu qtable.dat lúc
    // này), rồi mới chờ. Quá hạn thì giết để judge không bao giờ treo khi kết thúc.
    if (handles.input) {
        CloseHandle(handles.input);
        handles.input = nullptr;
    }
    if (handles.process) {
        if (WaitForSingleObject(handles.process, 10000) != WAIT_OBJECT_0)
            TerminateProcess(handles.process, 1);
        CloseHandle(handles.process);
        handles.process = nullptr;
    }
    if (handles.output) {
        CloseHandle(handles.output);
        handles.output = nullptr;
    }
}

}  // namespace winproc
