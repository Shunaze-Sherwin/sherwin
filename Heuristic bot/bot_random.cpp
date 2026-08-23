#pragma once
#include <bits/stdc++.h>
#include "rules.cpp" 
using namespace std;

// Bot Random: Liệt kê các nước đi hợp lệ và chọn ngẫu nhiên
char botRandom(const gamestate& state, bool isa, mt19937& rng) {
    Pos agent = isa ? state.a : state.b;
    string answer = "";
    char dirs[] = {'U', 'D', 'L', 'R', 'S'};
    
    for (char d : dirs) {
        if (canpush(state, agent, d)) {
            answer += d;
        }
    }
    
    if (answer.empty()) return 'S';
    
    // Rút thăm ngẫu nhiên 1 nước đi trong danh sách hợp lệ
    uniform_int_distribution<int> dist(0, answer.size() - 1);
    return answer[dist(rng)];
}