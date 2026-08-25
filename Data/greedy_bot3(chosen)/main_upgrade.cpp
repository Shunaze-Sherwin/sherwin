#include "upgrade.h"

// Entry point --interactive cho bot3 (không có sẵn trong 5 file được cung cấp).
// Đọc đúng giao thức 16 dòng bàn cờ + 2 điểm số như các bot khác trong repo này,
// dùng makeStateFromGrid() (game/state.cpp) để tách agent/hộp khỏi bản đồ tĩnh,
// rồi gọi botGreedy(state, true, timer) với ngân sách 1500ms/nước (judge.exe cho
// tối đa 2000ms/nước, xem bot_process_win.cpp:280).
int main(int argc, char **argv) {
    if (argc < 2 || string(argv[1]) != "--interactive") return 1;

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        vector<string> lines(16);
        for (string &line : lines) {
            if (!(cin >> line)) return 0;
        }

        Grid raw = inp(lines);
        gamestate state = makeStateFromGrid(raw);

        if (!(cin >> state.scoreA >> state.scoreB)) return 0;

        Timer timer(1500.0);
        cout << botGreedy(state, true, timer) << '\n';
        cout.flush();
    }
}
