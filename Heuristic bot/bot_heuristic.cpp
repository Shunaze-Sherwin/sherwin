#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "timer.cpp"
#include "evaluate.cpp"
#include "bot_random.cpp"
using namespace std;

// Bot Heuristic: Nhìn trước 1 bước
char botHeuristic(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    char dirs[] = {'U', 'D', 'L', 'R', 'S'};
    
    char bestMove = 'S';
    int bestQuality = -1e9; 
    for (char d : dirs) {
        if (canpush(state, agent, d)) {
            gamestate nextState = state;
            apply(nextState, isa, d);
            int quality = evaluatestate(nextState, isa);
            if (quality > bestQuality) {
                bestQuality = quality;
                bestMove = d;
            }
        }
    }
    
    return bestMove;
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
        cout << botHeuristic(state, true) << '\n';
        cout.flush();
    }
}
