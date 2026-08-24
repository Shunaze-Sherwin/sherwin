#pragma once
#include <bits/stdc++.h>
#include "../core/grid.cpp"
using namespace std;

struct Move {
    char dir; // 'U' (Lên), 'D' (Xuống), 'L' (Trái), 'R' (Phải), 'S' (Đứng yên)
};

// Cấu trúc Trạng thái của 1 ván đấu tại một thời điểm
struct gamestate {
    Grid grid;             // Bản đồ TĨNH: chỉ chứa '.', '#', 'A', 'B' (không chứa a/b/X)
    Pos a = {-1, -1};      // Tọa độ nhân vật phe ta
    Pos b = {-1, -1};      // Tọa độ nhân vật đối thủ
    vector<Pos> boxes;     // Danh sách tọa độ các hộp CÒN TRÊN BÀN
    int scoreA = 0;        // Điểm phe ta
    int scoreB = 0;        // Điểm phe địch

    bool hasBox(Pos p) const {
        for (const Pos& box : boxes) if (box == p) return true;
        return false;
    }

    // Di chuyển hộp từ oldPos sang newPos
    void update(Pos oldPos, Pos newPos) {
        for (auto& box : boxes) {
            if (box == oldPos) {
                box = newPos;
                return;
            }
        }
    }

    // Hộp đã vào đích -> XOÁ HẲN khỏi danh sách.
    // (Trước đây gán {-1,-1} và giữ lại, gây truy cập dist[-1][-1] ngoài mảng.)
    void removeBox(Pos p) {
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i] == p) {
                boxes.erase(boxes.begin() + i);
                return;
            }
        }
    }
};

// Tách các thực thể động (a, b, X) khỏi bản đồ tĩnh và nạp vào gamestate.
// Sau khi gọi, state.grid chỉ còn '.', '#', 'A', 'B'.
gamestate makeStateFromGrid(const Grid& raw) {
    gamestate st;
    st.grid = raw;
    st.boxes.clear();

    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            char ch = raw.cell[r][c];
            if (ch == 'a') { st.a = {r, c}; st.grid.cell[r][c] = '.'; }
            else if (ch == 'b') { st.b = {r, c}; st.grid.cell[r][c] = '.'; }
            else if (ch == 'X') { st.boxes.push_back({r, c}); st.grid.cell[r][c] = '.'; }
        }
    }
    return st;
}

// Dựng mặt nạ ô bị chiếm: mọi hộp + nhân vật đối phương.
Occupancy buildOccupancy(const gamestate& state, bool isa) {
    Occupancy occ = emptyOccupancy();
    for (const Pos& box : state.boxes) {
        if (state.grid.checkin(box)) occ[box.r][box.c] = true;
    }
    Pos other = isa ? state.b : state.a;
    if (state.grid.checkin(other)) occ[other.r][other.c] = true;
    return occ;
}

// Chữ ký trạng thái (vị trí agent + tập hộp đã sort) — dùng để phát hiện
// lặp trạng thái (dấu hiệu bot bị kẹt, "đổi ý giữa chừng" nhiều lần liên
// tiếp). Không cần hash phức tạp (kiểu Zobrist) ở quy mô bàn 16x16/vài
// nghìn lượt kiểm tra — so sánh string trực tiếp đủ nhanh (TASK ROBUST-03).
inline string stateSignature(const gamestate& state) {
    vector<Pos> sorted = state.boxes;
    sort(sorted.begin(), sorted.end(), [](const Pos& x, const Pos& y) {
        return x.r != y.r ? x.r < y.r : x.c < y.c;
    });
    string sig;
    sig.reserve(16 + sorted.size() * 8);
    sig += to_string(state.a.r) + "," + to_string(state.a.c) + "|";
    sig += to_string(state.b.r) + "," + to_string(state.b.c) + "|";
    for (const Pos& p : sorted) sig += to_string(p.r) + "," + to_string(p.c) + ";";
    return sig;
}

void printstate(const gamestate& state) {
    Grid displayGrid = state.grid;
    for (Pos box : state.boxes) {
        if (displayGrid.checkin(box)) displayGrid.cell[box.r][box.c] = 'X';
    }
    if (displayGrid.checkin(state.a)) displayGrid.cell[state.a.r][state.a.c] = 'a';
    if (displayGrid.checkin(state.b)) displayGrid.cell[state.b.r][state.b.c] = 'b';

    print(displayGrid);
    cout << "Diem A: " << state.scoreA << " | Diem B: " << state.scoreB
         << " | Hop con lai: " << state.boxes.size() << "\n";
    cout << "----------------------\n";
}
