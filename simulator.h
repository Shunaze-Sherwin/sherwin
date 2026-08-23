#pragma once
#include "common.h"
#include "quality.h"

//Aplly move-----------------------------------------------------------------------------------------------
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
        if (hash_table(next) != hash_table(current))
            res.push_back({move, next});
    }
    return res;
}

// check() chỉ giữ lại cặp (nước mình, nước địch) khi áp dụng theo 2 thứ tự khác nhau cho
// cùng kết quả (đối xứng) - nhưng luật thật (judge.cpp) luôn áp nước MÌNH trước, nước ĐỊCH
// sau theo thứ tự cố định, không có khái niệm đồng thời/đối xứng. Dùng nó làm bộ lọc ở đây
// vô tình loại bỏ nhiều phản ứng hợp lệ của địch khỏi minimax, khiến bot đánh giá thấp mối
// đe dọa thật. Đã bỏ khỏi vòng lặp bên dưới (giờ luôn xét đủ mọi cặp theo đúng thứ tự judge
// dùng); hàm vẫn giữ lại vì có thể cần dùng cho việc khác sau này.

bool check(const State &current, int my_move, int enemy_move){
    State a = apply_move(current, my_move, true);
    a = apply_move(a, enemy_move, false);
    State b = apply_move(current, enemy_move, false);
    b = apply_move(b, my_move, true);
    return hash_table(a) == hash_table(b);
}

// Tăng từ 10 (5 nước của mình) lên 14 (7 nước) - tầm nhìn 5 nước quá ngắn so với hành trình
// đi-tới-hộp-rồi-đẩy-về-đích trên bàn cờ 16x16, khiến quality() ở đáy cây không phân biệt
// được 4 hướng đi gốc (đã đo thực tế: quality bằng hệt nhau dù 4 hướng dẫn tới 4 vị trí khác
// nhau) -> bot chọn nước gần như tùy tiện, dao động qua lại thay vì tiến bộ. Từng thử 20 (10
// nước): mỗi nước ghim sát trần ngân sách 1.5s, quá gần giới hạn 2s judge chờ mỗi nước, không
// còn biên an toàn cho biến động hệ thống thực - 14 là điểm cân bằng giữ được biên an toàn đó.
int number_tick = 12;
pair<int, int> chosen_move = {-1, -1};

// Bỏ check() làm branching factor tăng lại đúng mức thật (tối đa 4x4/lượt), nên cần chặn
// thời gian để không vượt quá 2s judge chờ mỗi nước (judge.cpp poll timeout). Hết giờ thì
// coi node hiện tại như lá, trả quality() tĩnh thay vì tìm tiếp - suy biến an toàn, không
// bao giờ treo hay trả kết quả rác.
const chrono::milliseconds SEARCH_TIME_BUDGET(1500);
chrono::steady_clock::time_point search_deadline = chrono::steady_clock::now();

bool time_is_up() {
    return chrono::steady_clock::now() >= search_deadline;
}

// Nước đi thật sự đã chọn ở tick THẬT trước đó (không phải trong 1 lần tìm kiếm giả định) -
// dùng để phát hiện và phạt nhẹ việc đảo ngược ngay nước vừa đi. Khi nhiều ứng viên gốc hòa
// điểm tuyệt đối (đã đo thực tế: cả 4 hướng ra cùng 1 quality do tầm nhìn ngắn), bot trước đây
// chọn theo thứ tự cố định R,L,D,U - dễ tạo chu trình đi-rồi-quay-lại vô hạn. -1 nghĩa là
// chưa có nước trước đó (đầu game hoặc vừa đứng yên) nên không phạt gì.
int last_move = -1;

bool is_reverse_move(int a, int b) {
    if (a < 0 || b < 0) return false;
    return (a == 0 && b == 1) || (a == 1 && b == 0) ||
           (a == 2 && b == 3) || (a == 3 && b == 2);
}

ll simulator(const State &current, int tick, int root_tick,
             ll alpha = -1e18, ll beta = 1e18){
    if (tick > number_tick || time_is_up()) return quality(current);
    vector<Move> me = generate_moves(current, 1);
    vector<Move> enemy = generate_moves(current, 0);

    if (me.empty()) return quality(current);
    // Phạt nhẹ để chống đảo-ngược khi so sánh ứng viên GỐC - không đủ lớn để lật một nước
    // thực sự tốt hơn, chỉ đủ để phá hòa khi các ứng viên đang ngang điểm nhau.
    const ll REVERSAL_PENALTY = 10;

    if (enemy.empty()) {
        ll best_quality = -1e18;
        for (Move after_me : me) {
            ll candidate_quality = quality(after_me.nxt_state);
            if (tick == root_tick && is_reverse_move(after_me.move, last_move))
                candidate_quality -= REVERSAL_PENALTY;
            if (maximize(best_quality, candidate_quality) && tick == root_tick)
                chosen_move = {after_me.move, -1};
        }
        return best_quality;
    }

    ll best_quality = -1e18;
    for (Move after_me : me){
        pair<int, int> carry;
        carry.first = after_me.move;
        ll tmp = 1e18;
        for (Move after_enemy : enemy) {
            State next_2turn = apply_move(after_me.nxt_state, after_enemy.move, 0);
            ll nxt_quality = simulator(next_2turn, tick + 2, root_tick,
                                       alpha, beta);
            if (minimize(tmp, nxt_quality)) carry.second = after_enemy.move;
            if (tmp <= alpha) break;
            if (time_is_up()) break;
        }
        if (tmp != 1e18) {
            ll candidate_quality = tmp;
            if (tick == root_tick && is_reverse_move(after_me.move, last_move))
                candidate_quality -= REVERSAL_PENALTY;
            if (maximize(best_quality, candidate_quality) && tick == root_tick) chosen_move = carry;
        }
        maximize(alpha, best_quality);
        if (best_quality >= beta) break;
        if (time_is_up()) break;
    }
    return best_quality;
}