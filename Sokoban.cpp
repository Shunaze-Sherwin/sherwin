#include <bits/stdc++.h>

using namespace std;

typedef long long ll;

#define fu(i, a, b) for (int i = (a); i <= (b); ++i)
#define fd(i, a, b) for (int i = (a); i >= (b); --i)

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

ll Rand(ll l, ll r){
    return l + rng() % (r - l + 1);
}

//Load Input----------------------------------------------------------------------------------------
int target[17][17];
struct State{
    array<unsigned long long, 4> table;
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

    int get(int x, int y){
        if (x < 1 || x > 16 || y < 1 || y > 16) return 0;
        if (x == enemy.first && y == enemy.second) return 0;

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
} current;

void Load_input(){
    fu(i, 1, 16) fu(j, 1, 16) {
        char c; cin >> c;
        if (c == 'a') current.me = {i, j};
        if (c == 'b') current.enemy = {i, j};
        if (c == 'A') target[i][j] = 1;
        if (c == 'B') target[i][j] = -1;
        if (c == 'X') current.box.push_back({i, j});
        if (c != '#' && c != 'X' && c != 'a' && c != 'b' && c != 'A' && c != 'B') current.update(i, j, 1);
    }
}

//calculate quality-----------------------------------------------------------------------------------------------
string command[] = {"Down", "Up", "Right", "Left"};
int dx[] = {0, 0, 1, -1};
int dy[] = {1, -1, 0, 0};

bool safe(int x, int y, const State &a){
    return a.get(x, y) == 1;
}

array<array<int, 17>, 17> bfs(pair<int, int> start, const State &a){
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
            if (!safe(x, y, a) || cost[x][y] != -1) continue;
            cost[x][y] = cost[tmp.first][tmp.second] + 1;
            qu.push({x, y});
        }
    }
    return cost;
}

int dist(pair<int, int> start, pair<int, int> target){
    return 0;
}

//Aplly move-----------------------------------------------------------------------------------------------
bool find_box(const State &a, int x, int y){
    for (pair<int, int> tmp : a.box) if (tmp == make_pair(x, y)) return true;
    return false;
}

State apply_move(State current, int direction){
    int x = current.me.first + dx[direction];
    int y = current.me.second + dy[direction];

    if (current.get(x, y) == 1) {
        current.me = {x, y};
        return current;
    }

    for (int i = 0; i < current.box.size(); ++i){
        if (current.box[i] == make_pair(x, y)){
            int new_x = x + dx[direction];
            int new_y = y + dy[direction];
            int type = current.get(new_x, new_y);
            if (!type) return current;
            current.update(x, y, 1);
            if (type == 2 || type == -2) current.box.erase(current.box.begin() + i);
            else current.box[i] = {new_x, new_y}, current.update(new_x, new_y, 0);
            if (type == 2) ++current.my_score;
            if (type == -2) ++current.enemy_score;
            current.me = {x, y};
            break;
        }
    }
    return current;
}

//Convert state to number-----------------------------------------------------------------------------------------------
ll code[17][17];

void pre_hash_table(){
    int cnt = 0;
    fu(i, 1, 16) fu(j, 1, 16) code[i][j] = Rand(0, 1e14);
}

ll hash_vector(vector<pair<int, int>> &carry){
    ll res = 0;
    for (pair<int, int> tmp : carry) res ^= code[tmp.first][tmp.second];
    return res;
}

array<ll, 4> hash_table(State current){
    array<ll, 4> res = {0, 0, 0, 0};
    res[0] = current.me.first * 100 + current.me.second;
    res[1] = current.enemy.first * 100 + current.enemy.second;
    res[2] = hash_vector(current.box);
    res[3] = current.my_score - current.enemy_score;

    return res;
}

////Call_functions----------------------------------------------------------------------------------------
signed main(){
    #define name "history"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }
    #undef name

    #define name "current_test"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }

    ios_base::sync_with_stdio(false);
    cin.tie(nullptr); cout.tie(nullptr);

//Load Input----------------------------------------------------------------------------------------
    int number_table = 1;
    //cin >> numbertable;
    fu(i, 1, number_table) Load_input();
    pre_hash_table();

    map<array<ll, 4>, int> visted;
    queue<pair<State, int>> qu;
    qu.push({current, 0});
    visted[hash_table(current)] = 0;
}