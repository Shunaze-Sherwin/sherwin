#pragma once
#include "common.h"
#include "calculate_distance.h"

int goal_distance(pair<int, int> position, int goal_type){
    int best = 1e9;
    fu(i, 1, 16) fu(j, 1, 16) if (target[i][j] == goal_type)
        minimize(best, abs(position.first - i) + abs(position.second - j));
    return best;
}

bool is_dead_corner(const State &current, pair<int, int> position){
    if (target[position.first][position.second] != 0) return false;
    bool vertical = current.get(position.first - 1, position.second) == 0 ||
                    current.get(position.first + 1, position.second) == 0;
    bool horizontal = current.get(position.first, position.second - 1) == 0 ||
                      current.get(position.first, position.second + 1) == 0;
    return vertical && horizontal;
}

int quality(const State& current){
    int result = 100000 * (current.my_score - current.enemy_score);
    array<array<int, 17>, 17> my_distance = bfs(current.me, current);
    array<array<int, 17>, 17> enemy_distance = bfs(current.enemy, current);

    for (const pair<int, int> &box : current.box){
        int my_goal = goal_distance(box, 1);
        int enemy_goal = goal_distance(box, -1);
        result += 100 * (enemy_goal - my_goal);

        int my_pushes = 0;
        int enemy_pushes = 0;
        fu(direction, 0, 3){
            int stand_x = box.first - dx[direction];
            int stand_y = box.second - dy[direction];
            int next_x = box.first + dx[direction];
            int next_y = box.second + dy[direction];
            if (current.get(next_x, next_y) != 0){
                if (my_distance[stand_x][stand_y] != -1) ++my_pushes;
                if (enemy_distance[stand_x][stand_y] != -1) ++enemy_pushes;
            }
        }
        result += 25 * (my_pushes - enemy_pushes);
        if (is_dead_corner(current, box)) result -= 5000;
    }
    return result;
}