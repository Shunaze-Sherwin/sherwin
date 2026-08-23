#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "bfs.cpp" 
using namespace std;

// Bot Greedy: với mỗi hộp, xét cả 4 hướng đẩy khả dĩ, chọn (hộp, hướng đẩy) đưa hộp
// tới gần đích của MÌNH nhất, không bao giờ vô tình ghi điểm cho địch hay tự kẹt hộp
// vào góc chết, rồi mới di chuyển tới đúng vị trí đứng để thực hiện cú đẩy đó.
char botGreedy(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    char myGoal = isa ? 'A' : 'B';
    char enemyGoal = isa ? 'B' : 'A';

    vector<vector<int>> distToMyGoal = bfsToGoal(state.grid, myGoal);
    const char dirs[] = {'U', 'D', 'L', 'R'};

    int bestScore = INF;
    Pos bestStand = {-1, -1};
    char bestDir = 'S';
    bool found = false;

    for (Pos box : state.boxes) {
        vector<Pos> otherBoxes;
        for (Pos b : state.boxes) if (b != box) otherBoxes.push_back(b);

        for (char dir : dirs) {
            auto [dr, dc] = getidx(dir);
            Pos pushDest = {box.r + dr, box.c + dc};
            Pos standPos = {box.r - dr, box.c - dc};

            if (state.grid.isWall(pushDest) || state.grid.isWall(standPos)) continue;
            if (state.grid.cell[pushDest.r][pushDest.c] == enemyGoal) continue; // không tự đẩy hộp vào đích địch
            if (pushDest == other || standPos == other) continue;              // tránh vị trí bị agent địch chiếm
            if (isBlocked(otherBoxes, pushDest) || isBlocked(otherBoxes, standPos)) continue;
            if (distToMyGoal[pushDest.r][pushDest.c] == INF) continue;
            if (isDeadCorner(state.grid, pushDest)) continue;                  // tránh tự kẹt hộp vào góc chết

            vector<Pos> blockedForWalk = otherBoxes;
            blockedForWalk.push_back(other);
            int approachDist = bfsFrom(state.grid, agent, blockedForWalk)[standPos.r][standPos.c];
            if (approachDist == INF) continue;

            // Ưu tiên tiến độ hộp tới đích hơn quãng đường phải đi bộ.
            int score = distToMyGoal[pushDest.r][pushDest.c] * 4 + approachDist;
            if (score < bestScore) {
                bestScore = score;
                bestStand = standPos;
                bestDir = dir;
                found = true;
            }
        }
    }

    if (!found) return 'S';

    if (agent == bestStand) {
        return canpush(state, agent, bestDir) ? bestDir : 'S';
    }

    vector<Pos> blockedForWalk = state.boxes;
    blockedForWalk.push_back(other);
    char step = find(state.grid, agent, bestStand, blockedForWalk);

    return canpush(state, agent, step) ? step : 'S';
}

int main(int argc, char **argv) {
    if (argc < 2 || string(argv[1]) != "--interactive") return 1;

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        vector<string> lines(16);
        for (string &line : lines) {
            if (!(cin >> line)) return 0;
        }

        gamestate state;
        state.grid = inp(lines);
        for (int row = 0; row < 16; ++row) {
            for (int column = 0; column < 16; ++column) {
                char &cell = state.grid.cell[row][column];
                if (cell == 'a') {
                    state.a = {row, column};
                    cell = '.';
                } else if (cell == 'b') {
                    state.b = {row, column};
                    cell = '.';
                } else if (cell == 'X') {
                    state.boxes.push_back({row, column});
                    cell = '.';
                }
            }
        }

        if (!(cin >> state.scoreA >> state.scoreB)) return 0;
        cout << botGreedy(state, true) << '\n';
        cout.flush();
    }
}