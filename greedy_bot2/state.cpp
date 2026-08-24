#pragma once
#include <bits/stdc++.h>
#include "grid.cpp" 
using namespace std;

struct Move {
    char dir; // 'U' (Lên), 'D' (Xuống), 'L' (Trái), 'R' (Phải), 'S' (Đứng yên)
};

// Cấu trúc Trạng thái của 1 ván đấu tại một thời điểm
struct gamestate {
    Grid grid;             // Bản đồ gốc (chứa tường và các ô đích)
    Pos a = {-1, -1};      // Tọa độ nhân vật phe ta
    Pos b = {-1, -1};      // Tọa độ nhân vật đối thủ
    vector<Pos> boxes;     // Danh sách tọa độ các hộp hiện có trên bàn
    int scoreA = 0;        // Điểm phe ta
    int scoreB = 0;        // Điểm phe địch
    
    // Hàm tiện ích: Cập nhật tọa độ hộp sau khi bị đẩy
    void update(Pos oldPos, Pos newPos) {
        for (auto& box : boxes) {
            if (box == oldPos) {
                box = newPos;
                break;
            }
        }
    }
   
};
 void printstate(const gamestate& state) {
        Grid displayGrid = state.grid;
        for (Pos box : state.boxes) {
            if (box.r != -1) displayGrid.cell[box.r][box.c] = 'X';
        }
        displayGrid.cell[state.a.r][state.a.c] = 'a';
        displayGrid.cell[state.b.r][state.b.c] = 'b';
        
        print(displayGrid);
        cout << "Diem A: " << state.scoreA << " | Diem B: " << state.scoreB << "\n";
        cout << "----------------------\n";
    }
