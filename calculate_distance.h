#pragma once
#include "common.h"

bool safe(int x, int y, const State &current){
    return current.get(x, y) == 1;
}

array<array<int, 17>, 17> bfs(pair<int, int> start, const State &current){
    array<array<int, 17>, 17> cost;
    fu(i, 1, 16) fu(j, 1, 16) cost[i][j] = -1;
    queue<pair<int, int>> qu;
    qu.push(start);
    cost[start.first][start.second] = 0;

    while (!qu.empty()){
        pair<int, int> tmp = qu.front(); qu.pop();
        fu(i, 0, 3){
            int x = tmp.first + dx[i];
            int y = tmp.second + dy[i];
            if (!safe(x, y, current) || cost[x][y] != -1) continue;
            cost[x][y] = cost[tmp.first][tmp.second] + 1;
            qu.push({x, y});
        }
    }
    return cost;
}

int dist(pair<int, int> start, pair<int, int> target, const State &current){
    array<array<int, 17>, 17> cost = bfs(start, current);
    return cost[target.first][target.second];
}