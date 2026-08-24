#pragma once
#include <bits/stdc++.h>
using namespace std;

// Tọa độ trên bàn cờ
struct Pos {
    int r, c;
    bool operator==(const Pos& other) const {
        return r == other.r && c == other.c;
    }
    bool operator!=(const Pos& other) const {
        return !(*this == other);
    }
};
// Cấu trúc bàn cờ 16x16
struct Grid {
    char cell[16][16];

    // Kiểm tra tọa độ có nằm trong bàn cờ không
    bool checkin(Pos p) const {
        return p.r >= 0 && p.r < 16 && p.c >= 0 && p.c < 16;
    }

    // Kiểm tra ô có phải là tường không (ngoài biên mặc định là tường)
    bool isWall(Pos p) const {
        if (!checkin(p)) return true;
        return cell[p.r][p.c] == '#';
    }

    // Kiểm tra ô có thể đi vào được không (mọi ô không phải tường)
    bool isFree(Pos p) const {
        return checkin(p) && cell[p.r][p.c] != '#';
    }
};

// Hàm đọc bàn cờ từ dữ liệu đầu vào (giả định Input là 16 dòng string)
Grid inp(const vector<string>& lines) {
    Grid g;
    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            g.cell[r][c] = lines[r][c];
        }
    }
    return g;
}

// Hàm in bàn cờ ra màn hình để debug
void print(const Grid& g) {
    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            cout << g.cell[r][c];
        }
        cout << '\n';
    }
}

// Hàm sinh bản đồ ngẫu nhiên phục vụ Stress Test (Task 2)
