#pragma once
#include <bits/stdc++.h>
#include "../core/grid.cpp" 
using namespace std;

const int INF = 1e9; 

int dr[] = {-1, 1, 0, 0};
int dc[] = {0, 0, -1, 1};

// Hàm BFS: Trả về mảng 2D khoảng cách từ điểm xuất phát (start) đến mọi ô
vector<vector<int>> bfsFrom(const Grid& g, Pos start) {
    vector<vector<int>> dist(16, vector<int>(16, INF));
    
    if (g.isWall(start)) return dist;

    queue<Pos> q;
    q.push(start);
    dist[start.r][start.c] = 0;

    while (!q.empty()) {
        Pos u = q.front();
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Pos v = {u.r + dr[i], u.c + dc[i]};
            if (g.isFree(v) && dist[v.r][v.c] == INF) {
                dist[v.r][v.c] = dist[u.r][u.c] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}
// Đếm số ô "độc quyền" mà Agent A hoặc Agent B kiểm soát
pair<int, int> cnt(const Grid& g, Pos a, Pos b) {
    vector<vector<int>> distA = bfsFrom(g, a);
    vector<vector<int>> distB = bfsFrom(g, b);
    
    int scoreA = 0, scoreB = 0;
        for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            if (g.isWall({r, c})) continue; 
            
            if (distA[r][c] < distB[r][c]) {
                scoreA++; 
            } else if (distB[r][c] < distA[r][c]) {
                scoreB++;
            }
        }
    }
    return {scoreA, scoreB};
}
// Tìm hướng đi đầu tiên để di chuyển từ 'from' tới 'to' (U, D, L, R, hoặc S = Stay)
char find(const Grid& g, Pos from, Pos to) {
    if (from == to) return 'S';
    
    vector<vector<int>> dist = bfsFrom(g, to);
    if (dist[from.r][from.c] == INF) return 'S'; 
    
    int current_dist = dist[from.r][from.c];
    char dirs[] = {'U', 'D', 'L', 'R'}; 
    
    for (int i = 0; i < 4; ++i) {
        Pos next_pos = {from.r + dr[i], from.c + dc[i]};
        if (g.isFree(next_pos) && dist[next_pos.r][next_pos.c] == current_dist - 1) {
            return dirs[i]; 
        }
    }
    
    return 'S'; 
}