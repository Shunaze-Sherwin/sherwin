#pragma once
#include "common.h"
#include "calculate_distance.h"

int goal_dist[2][17][17];

struct QualityWeights {
    ll score = 100000;
    ll goal = 100;
    ll push = 25;
    ll dead_corner = 5000;
    ll approach = 50; // trọng số cho khoảng cách tới hộp đáng đẩy gần nhất - xem quality()
};

QualityWeights quality_weights;

void load_quality_weights(){
    ifstream input("weights.dat");
    if (!(input >> quality_weights.score >> quality_weights.goal
                >> quality_weights.push >> quality_weights.dead_corner)) {
        quality_weights = QualityWeights{};
        return;
    }
    if (!(input >> quality_weights.approach)) quality_weights.approach = 50;
}

bool static_free(const State &board, int x, int y){
    if (make_pair(x, y) == board.me || make_pair(x, y) == board.enemy) return true;
    if (find_box(board, x, y) != -1) return true;
    return board.get(x, y) == 1;
}

void init_quality(const State &board){
    load_quality_weights();
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
                int stand_x = x - dx[direction];
                int stand_y = y - dy[direction];
                if (!static_free(board, x, y) || !static_free(board, stand_x, stand_y) || goal_dist[type][x][y] != 1e9) continue;
                goal_dist[type][x][y] = goal_dist[type][tmp.first][tmp.second] + 1;
                qu.push({x, y});
            }
        }
    }
}

bool is_dead_corner(const State &, pair<int, int> pos){
    if (target[pos.first][pos.second] != 0) return false;
    bool vertical = is_wall(pos.first - 1, pos.second) || is_wall(pos.first + 1, pos.second);
    bool horizontal = is_wall(pos.first, pos.second - 1) || is_wall(pos.first, pos.second + 1);
    return vertical && horizontal;
}

// Khoảng cách đi bộ THẬT (BFS né tường/hộp/đối thủ) từ vị trí hiện tại tới ô cần đứng để
// đẩy hộp - trước đây dùng Manhattan distance (đường chim bay), khiến bot bị hút về phía
// một ô "gần" trên lý thuyết nhưng thực tế bị tường chắn, không có đường đi thật ngắn như
// vậy. walk_dist là bảng BFS đã tính sẵn 1 lần cho cả lượt gọi quality() (xem bfs() trong
// calculate_distance.h), truyền vào đây để tránh BFS lại cho từng hộp.
int distance_to_push(const State &current, pair<int, int> box, bool mine,
                      const array<array<int, 17>, 17> &walk_dist){
    int best = 1e9;
    int box_goal = goal_dist[mine ? 0 : 1][box.first][box.second];
    if (box_goal >= 1e9) return best;

    fu(direction, 0, 3){
        int next_x = box.first + dx[direction];
        int next_y = box.second + dy[direction];
        int stand_x = box.first - dx[direction];
        int stand_y = box.second - dy[direction];
        if (next_x < 1 || next_x > 16 || next_y < 1 || next_y > 16 ||
            stand_x < 1 || stand_x > 16 || stand_y < 1 || stand_y > 16)
            continue;
        if (goal_dist[mine ? 0 : 1][next_x][next_y] != box_goal - 1)
            continue;
        int walk = walk_dist[stand_x][stand_y];
        if (walk < 0) continue; // không có đường đi bộ thật tới ô đứng này
        minimize(best, walk);
    }
    return best;
}

ll quality(const State& current){
    ll res = quality_weights.score * (current.my_score - current.enemy_score);
    // Khoảng cách đi bộ tới hộp ĐÁNG ĐẨY NHẤT (nhỏ nhất trong các hộp), không cộng dồn qua
    // mọi hộp - cộng dồn sẽ bị các hộp xa (mà mình chưa nhắm tới) làm loãng tín hiệu "đang
    // tiến gần mục tiêu thật sự". Trước đây số này còn bị cộng thẳng không qua trọng số nào,
    // trong khi goal/push đã nhân với hệ số hàng trăm/nghìn - khiến việc tiến 1 bước gần hộp
    // gần như vô hình trước các thành phần khác, làm minimax không phân biệt được nước nào
    // thực sự tiến bộ ở tầm ngắn.
    // BFS 1 lần/bên cho cả lượt gọi quality() này, dùng chung cho mọi hộp bên dưới.
    array<array<int, 17>, 17> my_walk = bfs(current.me, current);
    array<array<int, 17>, 17> enemy_walk = bfs(current.enemy, current);
    int best_my_approach = (int)1e9, best_enemy_approach = (int)1e9;
    for (const pair<int, int> &box : current.box){
        int my_goal = goal_dist[0][box.first][box.second];
        int enemy_goal = goal_dist[1][box.first][box.second];
        res += quality_weights.goal * (enemy_goal - my_goal);

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
        res += quality_weights.push * (my_pushes - enemy_pushes);
        minimize(best_my_approach, distance_to_push(current, box, true, my_walk));
        minimize(best_enemy_approach, distance_to_push(current, box, false, enemy_walk));
        if (is_dead_corner(current, box)) res -= quality_weights.dead_corner;
    }
    if (best_my_approach < (int)1e9) res -= quality_weights.approach * best_my_approach;
    if (best_enemy_approach < (int)1e9) res += quality_weights.approach * best_enemy_approach;
    return res;
}