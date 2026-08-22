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

//Load Input----------------------------------------------------------------------------------------
int target[17][17];
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

State Load_input(){
    State res;
    fu(i, 1, 16) fu(j, 1, 16) {
        char c; cin >> c;
        if (c == 'a') res.me = {i, j};
        if (c == 'b') res.enemy = {i, j};
        if (c == 'A') target[i][j] = 1;
        if (c == 'B') target[i][j] = -1;
        if (c == 'X') res.box.push_back({i, j});
        if (c != '#' && c != 'X' && c != 'A' && c != 'B') res.update(i, j, 1);
    }
    return res;
}

//calculate distance-----------------------------------------------------------------------------------------------
string command[] = {"R", "L", "D", "U"};
int dx[] = {0, 0, 1, -1};
int dy[] = {1, -1, 0, 0};

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

//Aplly move-----------------------------------------------------------------------------------------------
bool find_box(const State &current, int x, int y){
    for (pair<int, int> tmp : current.box) if (tmp == make_pair(x, y)) return true;
    return false;
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

            if (my_turn) current.me = {x, y};
            else current.enemy = {x, y}; 
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
    res[3] = current.my_score * 1000 + current.enemy_score;

    return res;
}

//Simulator-----------------------------------------------------------------------------------------------
struct Move{
    int move;
    State nxt_state;
};

vector<Move> generate_moves(const State &current, bool my_turn){
    vector<Move> res;
    fu(move, 0, 3){
        State next = apply_move(current, move, my_turn);
        if (hash_table(next) == hash_table(current)) continue;
        res.push_back({move, next});
    }
    return res;
}

int quality(const State& current){
    return current.my_score - current.enemy_score;
}

int number_step = 10;
int chosen_move = -1;

int simulator(State &current, int tick, int root_tick){
    if (tick > number_step) return quality(current);
    bool my_turn  = tick & 1;
    vector<Move> moves = generate_moves(current, my_turn);
    if (moves.empty()) return quality(current);
    

    int best_quality = my_turn ? -1e9 : 1e9;
    for (Move tmp : moves){
        int nxt_quality = simulator(tmp.nxt_state, tick + 1, root_tick);
        if (my_turn) {
            if (maximize(best_quality, nxt_quality) && tick == root_tick)
                chosen_move = tmp.move;
        }
        else {
            if (minimize(best_quality, nxt_quality) && tick == root_tick)
                chosen_move = tmp.move;
        }
    }
    return best_quality;
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
    //cin >> numbertable; fu(i, 1, number_table) Load_input();
    State current = Load_input();
    pre_hash_table();
    
    fu(tick, 1, number_step){
        chosen_move = -1;
        simulator(current, tick, tick);
        bool my_turn = tick & 1;
        current = apply_move(current, chosen_move, my_turn);
        if (my_turn) cout << "my move: " << command[chosen_move] << endl;
        else cout << "enemy move: " << command[chosen_move] << endl;
    }
    cout << current.my_score << " " << current.enemy_score << endl;
}