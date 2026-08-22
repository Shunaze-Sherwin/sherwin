#pragma once
#include "common.h"
#include "calculate_distance.h"

int goal_dist[2][17][17];

bool static_free(const State &board, int x, int y){
    if (make_pair(x, y) == board.me || make_pair(x, y) == board.enemy) return true;
    return board.get(x, y);
}

void init_quality(const State &board){
    fu(type, 0, 1) fu(i, 1, 16) fu(j, 1, 16) goal_dist[type][i][j] = 1e9;

    fu(type, 0, 1){
        int goal_type = type == 0 ? 1 : -1;
        queue<pair<int, int>> qu;
        fu(i, 1, 16) fu(j, 1, 16){
            if (target[i][j] != goal_type) continue;
            goal_dist[type][i][j] = 0;
            qu.push({i, j});
        }
        while (!qu.empty()){
            pair<int, int> tmp = qu.front(); qu.pop();
            fu(direction, 0, 3){
                int x = tmp.first - dx[direction];
                int y = tmp.second - dy[direction];
                if (!static_free(board, x, y) || goal_dist[type][x][y] != 1e9) continue;
                goal_dist[type][x][y] = goal_dist[type][tmp.first][tmp.second] + 1;
                qu.push({x, y});
            }
        }
    }
}

bool is_dead_corner(const State &current, pair<int, int> pos){
    if (target[pos.first][pos.second] != 0) return false;
    bool vertical = current.get(pos.first - 1, pos.second) == 0 ||
                    current.get(pos.first + 1, pos.second) == 0;
    bool horizontal = current.get(pos.first, pos.second - 1) == 0 ||
                      current.get(pos.first, pos.second + 1) == 0;
    return vertical && horizontal;
}

int quality(const State& current){
    int result = 100000 * (current.my_score - current.enemy_score);
    for (const pair<int, int> &box : current.box){
        int my_goal = goal_dist[0][box.first][box.second];
        int enemy_goal = goal_dist[1][box.first][box.second];
        result += 100 * (enemy_goal - my_goal);

        int my_pushes = 0;
        int enemy_pushes = 0;
        fu(direction, 0, 3){
            int stand_x = box.first - dx[direction];
            int stand_y = box.second - dy[direction];
            int next_x = box.first + dx[direction];
            int next_y = box.second + dy[direction];
            if (current.get(next_x, next_y) != 0){
                int my_distance = abs(current.me.first - stand_x) + abs(current.me.second - stand_y);
                int enemy_distance = abs(current.enemy.first - stand_x) + abs(current.enemy.second - stand_y);
                if (my_distance <= 16) ++my_pushes;
                if (enemy_distance <= 16) ++enemy_pushes;
            }
        }
        result += 25 * (my_pushes - enemy_pushes);
        if (is_dead_corner(current, box)) result -= 5000;
    }
    return result;
}