#pragma once
#include <bits/stdc++.h>
#include "state.cpp"
using namespace std;

// Chuyển đổi ký tự hướng đi thành mức độ thay đổi tọa độ
pair<int, int> getidx(char dir) {
    if (dir == 'U') return {-1, 0};
    if (dir == 'D') return {1, 0};
    if (dir == 'L') return {0, -1};
    if (dir == 'R') return {0, 1};
    return {0, 0}; // 'S' - Đứng yên
}

// ========================================================
// CONFIGURABLE RULES (Sẽ sửa nếu đề thi thật có luật khác)
// ========================================================

// Kiểm tra xem một nước đi/đẩy hộp có hợp lệ không
bool canpush(const gamestate& state, Pos agent, char dir) {
    if (dir == 'S') return true;
    
    auto [dr, dc] = getidx(dir);
    Pos nextPos = {agent.r + dr, agent.c + dc};
    
    if (state.grid.isWall(nextPos)) return false;
    
    // 1.Kiểm tra xem ô tiếp theo có hộp không
    bool isBox = false;
    for (Pos box : state.boxes) {
        if (box == nextPos) isBox = true;
    }
    
    // 2. Nếu có hộp (đẩy 1 ô)
    if (isBox) {
        Pos pushpos = {nextPos.r + dr, nextPos.c + dc};
        
        // hộp đụng tường 
        if (state.grid.isWall(pushpos)) return false;
        
        // chống đẩy 2 hộp liên tiếp
        for (Pos box : state.boxes) {
            if (box == pushpos) return false;
        }
    }
    
    return true;
}

// Thực thi nước đi và chấm điểm
void apply(gamestate& state, bool isa, char dir) {
    if (dir == 'S') return;
    
    Pos& agent = isa ? state.a : state.b;
    auto [dr, dc] = getidx(dir);
    Pos nextPos = {agent.r + dr, agent.c + dc};
    
    // Tìm xem có đẩy trúng hộp nào không
    Pos boxToPush = {-1, -1};
    bool pushedBox = false;
    for (Pos box : state.boxes) {
        if (box == nextPos) {
            boxToPush = box;
            pushedBox = true;
            break;
        }
    }
    //cout << (pushedBox ? "[DAY HOP]" : "[DI CHUYEN]") << " Agent " << (isa ? "A" : "B") << " di chuyen " << dir << "\n";
    // Nếu có đẩy hộp, di chuyển hộp và chấm điểm
    if (pushedBox) {
        Pos newBoxPos = {nextPos.r + dr, nextPos.c + dc};
        if (state.grid.cell[newBoxPos.r][newBoxPos.c] == 'A') {
            state.scoreA++;
            state.update(boxToPush, {-1, -1}); 
        } else if (state.grid.cell[newBoxPos.r][newBoxPos.c] == 'B') {
            state.scoreB++;
            state.update(boxToPush, {-1, -1});
        } else {
            state.update(boxToPush, newBoxPos);
        }
    }
    
    agent = nextPos;
}   