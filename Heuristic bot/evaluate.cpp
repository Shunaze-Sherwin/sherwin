#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "bfs.cpp" 
using namespace std;

// Hàm chấm điểm (Quality)
int evaluatestate(const gamestate& state, bool isa) {
    int quality = 0;
    
    // ==========================================
    // FEATURE 1: Chênh lệch điểm số (Trọng số: 1000)
    // ==========================================
    int myScore = isa ? state.scoreA : state.scoreB;
    int oppScore = isa ? state.scoreB : state.scoreA;
    quality += (myScore - oppScore) * 1000;
    
    if (state.boxes.empty() || state.boxes[0].r == -1) {
        return quality; 
    }
    
    Pos agent = isa ? state.a : state.b;
    Pos box = state.boxes[0]; 

    // ==========================================
    // FEATURE 2: Cu ly tu Hop -> Dich cua minh (Trong so: 50)
    // Day la feature quyet dinh: khong co no thi bot khong bao gio
    // biet day hop VE HUONG NAO la co loi, chi biet lai gan hop.
    // ==========================================
    char myGoalChar = isa ? 'A' : 'B';
    Pos goal = {-1, -1};
    for (int r = 0; r < 16 && goal.r == -1; ++r)
        for (int c = 0; c < 16; ++c)
            if (state.grid.cell[r][c] == myGoalChar) { goal = {r, c}; break; }

    if (goal.r != -1) {
        vector<vector<int>> distFromGoal = bfsFrom(state.grid, goal);
        if (distFromGoal[box.r][box.c] != INF) {
            quality -= distFromGoal[box.r][box.c] * 50;
        }
    }

    // ==========================================
    // FEATURE 3: Cu ly Nhan vat -> Hop (Trong so: 10)
    // ==========================================
    vector<vector<int>> distFromAgent = bfsFrom(state.grid, agent);
    if (distFromAgent[box.r][box.c] != INF) {
        quality -= distFromAgent[box.r][box.c] * 10; 
    }
    
    return quality;
}