#include <bits/stdc++.h>

using namespace std;

#define fu(i, a, b) for (int i = (a); i <= (b); ++i)
#define fd(i, a, b) for (int i = (a); i >= (b); --i)

//Load Input----------------------------------------------------------------------------------------
struct State{
    array<unsigned long long, 4> table;
    pair<int, int> me;
    pair<int, int> enemy;
    vector<pair<int, int>> box;
    vector<pair<int, int>> my_target;
    vector<pair<int, int>> enemy_target;

    void update(int x, int y){
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        table[x] |= (1ll << y);
    }

    int get(int x, int y){
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        return (table[x] >> y) & 1ll;
    }
} current;

void Load_input(){
    fu(i, 1, 16) fu(j, 1, 16) {
        char c; cin >> c;
        if (c == 'a') current.me = {i, j};
        if (c == 'b') current.enemy = {i, j};
        if (c == 'X') current.box.push_back({i, j});
        if (c == 'A') current.my_target.push_back({i, j});
        if (c == 'B') current.enemy_target.push_back({i, j});
        if (c != '#' && c != 'X') current.update(i, j);
    }
}

//calculate quality-----------------------------------------------------------------------------------------------
string command[] = {"Down", "Up", "Right", "Left"};
int dx[] = {0, 0, 1, -1};
int dy[] = {1, -1, 0, 0};

bool safe(int x, int y, int cost[][17]){
    return x >= 1 && x <= 16 && y >= 1 && y <= 16 && current.get(x, y) && cost[x][y] == -1;
}

void bfs(pair<int, int> start, int cost[][17]){
    queue<pair<int, int>> qu;
    qu.push(start);
    cost[start.first][start.second] = 0;

    while (!qu.empty()){
        pair<int, int> tmp = qu.front(); qu.pop();
        cout << tmp.first << ' ' << tmp.second << '\n';
        fu(i, 0, 3){
            int x = tmp.first + dx[i];
            int y = tmp.second + dy[i];
            if (!safe(x, y, cost)) continue;
            cost[x][y] = cost[tmp.first][tmp.second] + 1;
            qu.push({x, y});
        }
    }
}

int dist(pair<int, int> start, pair<int, int> target){
    int cost[17][17]; memset(cost, -1, sizeof(cost));
    bfs(start, cost);
    return cost[target.first][target.second];
}

//Aplly move-----------------------------------------------------------------------------------------------
State apply_move(State current, int direction){
    int x = current.me.first + dx[direction];
    int y = current.me.second + dy[direction];

    if (current.get(x, y) == 0) return current;

    for (int i = 0; i < current.box.size(); ++i){
        if (current.box[i] == make_pair(x, y)){
            int new_x = x + dx[direction];
            int new_y = y + dy[direction];
            if (current.get(new_x, new_y) == 0) return current;
            current.box[i] = {new_x, new_y};
            break;
        }
    }

    current.me = {x, y};
    return current;
}

////Call_functions----------------------------------------------------------------------------------------
signed main(){
    #define name "history"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }

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
    
}