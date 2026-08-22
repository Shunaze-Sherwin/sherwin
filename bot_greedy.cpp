#pragma once
#include <bits/stdc++.h>
#include "../game/rules.cpp"
#include "../search/bfs.cpp" 
using namespace std;

// Bot Greedy: Tìm hộp gần nhất và tiến tới đẩy
char botGreedy(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    
    vector<vector<int>> dist = bfsFrom(state.grid, agent);
    
    int mindist = INF;
    Pos targetBox = {-1, -1};
    for (Pos box : state.boxes) {
        if (dist[box.r][box.c] < mindist) {
            mindist = dist[box.r][box.c];
            targetBox = box;
        }
    }
    
    if (targetBox.r == -1) return 'S';
    
    char step = find(state.grid, agent, targetBox);
    
    if (canpush(state, agent, step)) {
        return step;
    }
    
    return 'S';     
}