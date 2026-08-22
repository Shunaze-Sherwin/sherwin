#pragma once
#include "common.h"
#include "quality.h"

//Aplly move-----------------------------------------------------------------------------------------------
int find_box(const State &current, int x, int y){
    fu(i, 0, (int)current.box.size() - 1) if (current.box[i] == make_pair(x, y)) return i;
    return -1;
}

State apply_move(State current, int direction, bool my_turn){
    int x, y;
    if (my_turn) {
        x = current.me.first + dx[direction];
        y = current.me.second + dy[direction];
    } else {
        x = current.enemy.first + dx[direction];
        y = current.enemy.second + dy[direction];
    }

    if (current.get(x, y) == 1) {
        if (my_turn) current.me = {x, y};
        else current.enemy = {x, y};
        return current;
    }
    int pos = find_box(current, x, y);
    if (pos == -1) return current;

    int new_x = x + dx[direction];
    int new_y = y + dy[direction];
    int type = current.get(new_x, new_y);
    if (!type) return current;
    current.update(x, y, 1);
    if (type == 2 || type == -2) current.box.erase(current.box.begin() + pos);
    else current.box[pos] = {new_x, new_y}, current.update(new_x, new_y, 0);
    if (type == 2) ++current.my_score;
    if (type == -2) ++current.enemy_score;
    if (my_turn) current.me = {x, y};
    else current.enemy = {x, y}; 

    return current;
}

//Simulate--------------------------------------------------------------------------------------------------
struct Move{
    int move;
    State nxt_state;
};

vector<Move> generate_moves(const State &current, bool my_turn){
    vector<Move> res;
    fu(move, 0, 3){
        State next = apply_move(current, move, my_turn);
        res.push_back({move, next});
    }
    return res;
}

bool check(const State &current, int my_move, int enemy_move){
    State a = apply_move(current, my_move, true);
    a = apply_move(a, enemy_move, false);
    State b = apply_move(current, enemy_move, false);
    b = apply_move(b, my_move, true);
    return hash_table(a) == hash_table(b);
}

int number_tick = 10;
pair<int, int> chosen_move = {-1, -1};

int simulator(const State &current, int tick, int root_tick){
    if (tick > number_tick) return quality(current);
    vector<Move> me = generate_moves(current, 1);
    vector<Move> enemy = generate_moves(current, 0);
    if (me.empty() && enemy.empty()) return quality(current);

    int best_quality = -1e9;
    for (Move after_me : me){
        pair<int, int> carry;
        carry.first = after_me.move;
        int tmp = 1e9;
        for (Move after_enemy : enemy) if (check(current, after_me.move, after_enemy.move)){
            State next_2turn = apply_move(after_me.nxt_state, after_enemy.move, 0);
            int nxt_quality = simulator(next_2turn, tick + 2, root_tick);
            if (minimize(tmp, nxt_quality)) carry.second = after_enemy.move;
        }
        if (tmp != 1e9 && maximize(best_quality, tmp) && tick == root_tick) chosen_move = carry;
    }
    return best_quality;
}