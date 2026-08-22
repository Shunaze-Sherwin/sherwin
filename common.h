#pragma once
#include <bits/stdc++.h>

using namespace std;

typedef long long ll;

#define fu(i, a, b) for (int i = (a); i <= (b); ++i)
#define fd(i, a, b) for (int i = (a); i >= (b); --i)

template<typename T> inline bool minimize(T &a, const T &b){return a > b ? a = b, 1 : 0;}
template<typename T> inline bool maximize(T &a, const T &b){return a < b ? a = b, 1 : 0;}

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

ll Rand(ll l, ll r){
    return l + rng() % (r - l + 1);
}

string command[] = {"R", "L", "D", "U"};
int dx[] = {0, 0, 1, -1};
int dy[] = {1, -1, 0, 0};

//State---------------------------------------------------------------------------------------
int target[17][17];
bool wall[17][17];
struct State{
    array<unsigned long long, 4> table{};
    pair<int, int> me;
    pair<int, int> enemy;
    vector<pair<int, int>> box;
    int my_score = 0;
    int enemy_score = 0;

    void update(int x, int y, int val){
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        if (val) table[x] |= (1ull << y);
        else table[x] &= ~(1ull << y);
    }

    int get(int x, int y) const{
        if (x < 1 || x > 16 || y < 1 || y > 16) return 0;
        if (x == enemy.first && y == enemy.second) return 0;
        if (x == me.first && y == me.second) return 0;

        if (target[x][y] == 1) return 2;
        if (target[x][y] == -1) return -2;
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        return (table[x] >> y) & 1ull;
    }
    // 0: wall or something on it
    // 1: empty
    // 2: my point
    // -2: enemy's point
};

bool Load_input(State &res){
    res = State{};
    fu(i, 1, 16) fu(j, 1, 16) {
        char c;
        if (!(cin >> c)) return false;
        if (c == 'a') res.me = {i, j};
        if (c == 'b') res.enemy = {i, j};
        if (c == 'A') target[i][j] = 1;
        if (c == 'B') target[i][j] = -1;
        if (c == 'X') res.box.push_back({i, j});
        if (c == '#') wall[i][j] = 1;
        if (c != '#' && c != 'X' && c != 'A' && c != 'B') res.update(i, j, 1);
    }
    int my_score, enemy_score;
    if (cin >> my_score >> enemy_score){
        res.my_score = my_score;
        res.enemy_score = enemy_score;
    }
    return true;
}

State Load_input(){
    State result;
    Load_input(result);
    return result;
}

bool is_wall(int x, int y){
    if (x < 1 || y < 1 || x > 16 || y > 16) return true;
    return wall[x][y];
}

int find_box(const State &current, int x, int y){
    fu(i, 0, (int)current.box.size() - 1) if (current.box[i] == make_pair(x, y)) return i;
    return -1;
}

//Hash State---------------------------------------------------------------------------------
ll code[17][17];

void pre_hash_table(){
    fu(i, 1, 16) fu(j, 1, 16) code[i][j] = Rand(0, 1e14);
}

ll hash_vector(const vector<pair<int, int>> &carry){
    ll res = 0;
    for (pair<int, int> tmp : carry) res ^= code[tmp.first][tmp.second];
    return res;
}

array<ll, 4> hash_table(const State &current){
    array<ll, 4> res = {0, 0, 0, 0};
    res[0] = current.me.first * 100 + current.me.second;
    res[1] = current.enemy.first * 100 + current.enemy.second;
    res[2] = hash_vector(current.box);
    res[3] = current.my_score * 1000 + current.enemy_score;

    return res;
}