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
