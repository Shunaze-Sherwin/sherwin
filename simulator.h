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

// judge.cpp áp 2 nước đi ĐỒNG THỜI trên CÙNG bàn gốc (không bên nào thấy bàn đã bị bên kia
// đổi trước); nếu 2 nước THỰC SỰ tranh chấp (cùng ô, hoặc cùng đẩy 1 hộp), HUỶ CẢ HAI, không
// ai được lợi thế thứ tự (xem apply_moves() trong judge.cpp). Trước đây simulator() luôn áp
// nước MÌNH trước rồi coi nó chắc chắn thành công, sau đó mới cho địch phản ứng trên bàn đã
// đổi - bot không hề biết nước của mình có thể bị huỷ do đụng độ, nên khi 2 bên (bot thật lẫn
// đối thủ) liên tục chọn 2 nước tranh chấp nhau, bàn cờ đứng yên vĩnh viễn mà bot vẫn cứ chọn
// lại đúng nước cũ mỗi tick vì bàn (theo bot) không hề thay đổi - kẹt tới hết ván. plan_move/
// moves_conflict/apply_both_moves dưới đây mô phỏng lại đúng luật đó để minimax thấy trước và
// né (hoặc khai thác) được xung đột thay vì mắc kẹt rồi mới biết.
struct MovePlan{
    bool valid = false;
    pair<int, int> agent_to{-1, -1};
    bool pushes_box = false;
    pair<int, int> box_from{-1, -1}, box_to{-1, -1};
    int scores = 0; // 2 nếu hộp vào đích mình, -2 nếu vào đích địch, 0 nếu không ghi điểm
};

MovePlan plan_move(const State &current, int direction, bool my_turn){
    MovePlan plan;
    pair<int, int> position = my_turn ? current.me : current.enemy;
    int x = position.first + dx[direction];
    int y = position.second + dy[direction];

    if (current.get(x, y) == 1) {
        plan.valid = true;
        plan.agent_to = {x, y};
        return plan;
    }
    int pos = find_box(current, x, y);
    if (pos == -1) return plan;

    int new_x = x + dx[direction];
    int new_y = y + dy[direction];
    int type = current.get(new_x, new_y);
    if (!type) return plan;

    plan.valid = true;
    plan.agent_to = {x, y};
    plan.pushes_box = true;
    plan.box_from = {x, y};
    plan.box_to = {new_x, new_y};
    if (type == 2 || type == -2) plan.scores = type;
    return plan;
}

vector<pair<int, int>> claimed_cells(const MovePlan &plan){
    vector<pair<int, int>> cells;
    if (!plan.valid) return cells;
    cells.push_back(plan.agent_to);
    if (plan.pushes_box && !plan.scores) cells.push_back(plan.box_to);
    return cells;
}

bool moves_conflict(const MovePlan &mine, const MovePlan &enemy){
    if (!mine.valid || !enemy.valid) return false;
    if (mine.pushes_box && enemy.pushes_box && mine.box_from == enemy.box_from) return true;
    for (pair<int, int> a : claimed_cells(mine))
        for (pair<int, int> b : claimed_cells(enemy))
            if (a == b) return true;
    return false;
}

// Không tranh chấp nghĩa là 2 thay đổi độc lập, không chồng ô/hộp - luật đẩy hộp vốn đã cấm
// đẩy hộp vào ô đang có hộp khác, nên áp tuần tự qua apply_move() (mình rồi tới địch) cho
// đúng kết quả y hệt áp đồng thời, không cần viết lại logic ghi điểm/di chuyển hộp lần 2.
State apply_both_moves(const State &current, int my_move, int enemy_move){
    MovePlan mine = plan_move(current, my_move, true);
    MovePlan enemy = plan_move(current, enemy_move, false);
    if (moves_conflict(mine, enemy)) return current;
    State next = current;
    if (mine.valid) next = apply_move(next, my_move, true);
    if (enemy.valid) next = apply_move(next, enemy_move, false);
    return next;
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
            State next_2turn = apply_both_moves(current, after_me.move, after_enemy.move);
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