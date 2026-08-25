#pragma once
#include <bits/stdc++.h>
#include "grid.cpp"
using namespace std;

const int INF = 1e9;

int dr[] = {-1, 1, 0, 0};
int dc[] = {0, 0, -1, 1};
const char DIRCH[] = {'U', 'D', 'L', 'R'};

// (Occupancy / emptyOccupancy được định nghĩa trong core/grid.cpp)

// BFS cơ bản: chỉ né tường. Dùng để ước lượng khoảng cách của HỘP (hộp đẩy được qua ô có agent).
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

// BFS cho AGENT: né tường VÀ né các ô bị chiếm (hộp, nhân vật đối phương).
// Ô xuất phát luôn hợp lệ kể cả khi đang bị đánh dấu chiếm.
vector<vector<int>> bfsAvoid(const Grid& g, Pos start, const Occupancy& occ) {
    vector<vector<int>> dist(16, vector<int>(16, INF));

    if (!g.isFree(start)) return dist;

    queue<Pos> q;
    q.push(start);
    dist[start.r][start.c] = 0;

    while (!q.empty()) {
        Pos u = q.front();
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Pos v = {u.r + dr[i], u.c + dc[i]};
            if (!g.isFree(v)) continue;
            if (occ[v.r][v.c]) continue;
            if (dist[v.r][v.c] != INF) continue;
            dist[v.r][v.c] = dist[u.r][u.c] + 1;
            q.push(v);
        }
    }
    return dist;
}

// BFS đa nguồn: khoảng cách từ MỌI ô tới ô đích gần nhất mang ký tự goalChar ('A' hoặc 'B').
// Dùng để đo "hộp này còn cách đích của mình bao xa".
vector<vector<int>> bfsToGoals(const Grid& g, char goalChar) {
    vector<vector<int>> dist(16, vector<int>(16, INF));
    queue<Pos> q;

    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            if (g.cell[r][c] == goalChar) {
                dist[r][c] = 0;
                q.push({r, c});
            }
        }
    }

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

// Bước đi đầu tiên từ 'from' tới 'to', né các ô bị chiếm. Trả 'S' nếu không tới được.
// (Đổi tên từ find() để không đụng std::find khi có using namespace std.)
char firstStepTowards(const Grid& g, Pos from, Pos to, const Occupancy& occ) {
    if (from == to) return 'S';
    if (!g.isFree(to) || occ[to.r][to.c]) return 'S';

    vector<vector<int>> dist = bfsAvoid(g, to, occ);
    if (dist[from.r][from.c] == INF) return 'S';

    int current_dist = dist[from.r][from.c];

    for (int i = 0; i < 4; ++i) {
        Pos next_pos = {from.r + dr[i], from.c + dc[i]};
        if (!g.isFree(next_pos)) continue;
        if (occ[next_pos.r][next_pos.c]) continue;
        if (dist[next_pos.r][next_pos.c] == current_dist - 1) {
            return DIRCH[i];
        }
    }

    return 'S';
}

// Bản tiện lợi: không có ô bị chiếm nào
char firstStepTowards(const Grid& g, Pos from, Pos to) {
    return firstStepTowards(g, from, to, emptyOccupancy());
}
