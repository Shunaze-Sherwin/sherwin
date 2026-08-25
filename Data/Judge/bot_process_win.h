#pragma once
#include <string>

// Lớp mỏng bọc việc chạy bot con + ống dẫn 2 chiều trên Windows, thay cho
// fork()/pipe()/poll() của POSIX. CỐ Ý không #include <windows.h> ở đây: windef.h
// định nghĩa typedef `SIZE`, đụng độ với hằng `const int SIZE = 16` của judge.cpp
// (typedef-name và tên biến cùng phạm vi là lỗi biên dịch, khác với tên struct vốn
// được phép bị biến che). Vì vậy các HANDLE được giấu sau void*, còn windows.h chỉ
// xuất hiện trong bot_process_win.cpp.
namespace winproc {

struct Handles {
    void *process = nullptr;
    void *input = nullptr;   // đầu GHI, nối vào stdin của bot
    void *output = nullptr;  // đầu ĐỌC, nối từ stdout của bot
};

// Chạy `bot` với tham số --interactive. Nếu `bot` không có đuôi .exe mà tồn tại
// file "<bot>.exe" thì ưu tiên file đó, để vẫn gọi được `judge Sokoban greedy_bot`
// y hệt trên Linux dù bản Windows sinh ra Sokoban.exe/greedy_bot.exe.
bool spawn(const char *bot, Handles &handles);

bool write_all(const Handles &handles, const std::string &data);

// Đọc tới hết 1 dòng (không kèm '\n'). Trả false nếu quá `timeout_ms` mà bot chưa
// trả lời, hoặc ống đã đóng - tương đương poll() trả <= 0 ở bản POSIX.
bool read_line(const Handles &handles, int timeout_ms, std::string &line);

void destroy(Handles &handles);

}  // namespace winproc
