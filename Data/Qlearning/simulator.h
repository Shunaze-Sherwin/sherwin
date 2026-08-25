#pragma once
#include "common.h"
#include "quality.h"
#include <thread>
#include <atomic>

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
    res.reserve(4);
    // hash_table(current) không đổi suốt 4 lần lặp - trước đây tính lại mỗi vòng
    // (duyệt lại toàn bộ danh sách hộp mỗi lần), giờ tính 1 lần rồi dùng chung.
    array<ll, 4> current_hash = hash_table(current);
    fu(move, 0, 3){
        State next = apply_move(current, move, my_turn);
        if (hash_table(next) != current_hash)
            res.push_back({move, std::move(next)});
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

// Đã thử nâng lên 16 (an toàn về thời gian - đã đo thực tế lặp lại 15 lần trên bàn cờ
// khó nhất có sẵn: tick=12 max 1538ms, tick=16 max 1523ms, gần như giống hệt nhau vì
// time_is_up() được kiểm tra ở MỌI nút bất kể number_tick nên trần 1.5s luôn được tôn
// trọng). Nhưng đổi lại: bàn cờ ĐỦ dễ (trước đây duyệt hết cây rồi dừng sớm dưới 1.5s
// ở mức 12) giờ có việc để làm tới tận gần 1.5s - benchmark 300 tick cho thấy trung
// bình mỗi nước tăng từ 1.05s lên 1.53s. Đó là đánh đổi TỐC ĐỘ lấy chất lượng (nhìn xa
// hơn ở bàn dễ), không phải lỗi - nhưng ưu tiên hiện tại là tốc độ nên giữ lại 12.
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

// Lịch sử ~20 vị trí THẬT gần nhất của bot (ghi mỗi tick THẬT trong run_search(), không phải
// trong search giả định) - đã xác nhận thực nghiệm (workflow A/B: penalty 10 vs 3000, 40 ván,
// cùng qtable.dat thật) rằng REVERSAL_PENALTY - dù tăng lên bao nhiêu - KHÔNG bắt được chu
// trình lặp vị trí dạng có tick đệm (vd L,D,R,D,L,D,R,...) vì nó chỉ so với last_move (1 nước
// NGAY TRƯỚC), mà nước ngay trước mỗi lần đảo chiều luôn là tick đệm D/U chứ không phải chính
// hướng bị đảo - điều kiện kích hoạt phạt không bao giờ xảy ra. Cơ chế dưới đây chặn TỪ GỐC dựa
// trên VỊ TRÍ THỰC TẾ đã đứng, không phụ thuộc chuỗi nước đi cụ thể nên bắt được mọi chu kỳ bất
// kể có đệm hay không, miễn bot quay lại đúng ô cũ.
// 20 ban đầu KHÔNG đủ - đã xác nhận thực nghiệm (workflow kiểm chứng, 25 ván) chu kỳ vẫn còn
// gần như nguyên vẹn (100% ván heuristic_bot, 90% ván greedy_bot vẫn dính) vì các đoạn quan sát
// được dài phổ biến ~15 tick: đi hết 1 đoạn 15 tick rồi quay lại thì vị trí xuất phát đã rớt
// khỏi cửa sổ 20. Nâng lên 100 - chi phí kiểm tra chỉ là quét tuyến tính ~100 phần tử cho tối đa
// 4 ứng viên mỗi tick, không đáng kể so với ngân sách tìm kiếm 1.5s.
const size_t POSITION_HISTORY_SIZE = 100;
deque<pair<int, int>> recent_positions;

void record_position(pair<int, int> pos) {
    recent_positions.push_back(pos);
    if (recent_positions.size() > POSITION_HISTORY_SIZE) recent_positions.pop_front();
}

bool visited_recently(pair<int, int> pos) {
    for (const pair<int, int> &p : recent_positions) if (p == pos) return true;
    return false;
}

// Nước có đẩy hộp KHÔNG bao giờ bị chặn dù dẫn về ô cũ - đẩy hộp là công việc thật (vd đi vòng
// qua hộp để đẩy từ phía đối diện), không phải dậm chân tại chỗ. Chỉ nước đi bộ thuần tuý mới
// đáng ngờ khi quay lại vị trí đã đứng gần đây.
bool move_pushes_box(const State &current, int direction) {
    int x = current.me.first + dx[direction];
    int y = current.me.second + dy[direction];
    if (current.get(x, y) == 1) return false; // ô trống - đi bộ, không đẩy gì
    return find_box(current, x, y) != -1; // generate_moves() đã lọc hợp lệ nên chắc chắn là đẩy hộp
}

// Máy có 4 lõi CPU nhưng bản trước chạy đơn luồng: mỗi tick, minimax luôn ghim sát
// trần SEARCH_TIME_BUDGET (1.5s) vì cây (nhánh tới 4x4/lượt, sâu 12) gần như không
// bao giờ duyệt hết trong ngần ấy thời gian, bất kể code bên trong nhanh cỡ nào - tối
// ưu cài đặt (BoxList, BFS mảng tĩnh...) chỉ giúp duyệt được NHIỀU NÚT HƠN trong 1.5s
// đó chứ không hạ thấp trần. Ở đây chia 4 ứng viên nước đi GỐC của mình (tối đa 4:
// R/L/D/U) cho tối đa 4 luồng, mỗi luồng tìm phản ứng tệ nhất của địch cho ĐÚNG 1 ứng
// viên một cách ĐỘC LẬP hoàn toàn (State riêng, không đụng State/kết quả của luồng
// khác). Nhờ vậy: (1) bàn cờ "dễ" (cây duyệt hết trước 1.5s, đa số các tick theo đo
// thực tế) nhanh hơn tới ~4 lần vì 4 ứng viên chạy CÙNG LÚC thay vì lần lượt; (2) bàn
// cờ "khó" (cần cả 1.5s) tuy thời gian tổng không giảm (đúng thiết kế: 1.5s là trần cố
// ý, không phải bug), nhưng CHẤT LƯỢNG nước đi còn tốt hơn bản cũ - trước đây khi hết
// giờ giữa chừng, vòng lặp tuần tự bỏ luôn các ứng viên CHƯA xét tới (luôn theo thứ tự
// cố định R,L,D,U nên L/D/U dễ bị thiệt), giờ CẢ 4 ứng viên đều được cấp đủ 1.5s công
// bằng như nhau, không còn thiên vị theo thứ tự.
//
// shared_alpha: cận dưới (alpha) tốt nhất đã CHẮC CHẮN đạt được, cập nhật bởi bất kỳ
// luồng gốc nào đã tính XONG hoàn chỉnh ứng viên của nó (xem evaluate_root_candidate).
// Mọi luồng ANH EM còn đang chạy đọc giá trị này ở MỌI nút (dòng maximize() ngay dưới)
// để cắt tỉa sớm hơn - đúng ý nghĩa alpha-beta tuần tự (nếu xử lý candidate A trước rồi
// mới tới B, alpha=giá_trị(A) được truyền xuống XUYÊN SUỐT toàn bộ cây con của B ngay
// từ đầu); ở đây chỉ khác là "trước" được quyết định bởi luồng nào XONG trước, không
// phải thứ tự R,L,D,U cố định, và giá trị có thể cải thiện THÊM giữa chừng nếu một
// luồng khác vừa xong với kết quả tốt hơn. reset về -inf ở đầu mỗi run_search() nên
// không rò rỉ giữa các tick; memory_order_relaxed là đủ vì đây chỉ là 1 số nguyên độc
// lập dùng để cắt tỉa (best-effort), không đồng bộ hoá bất kỳ dữ liệu nào khác.
atomic<ll> shared_alpha(-1e18);

ll simulator(const State &current, int tick, ll alpha = -1e18, ll beta = 1e18){
    if (tick > number_tick || time_is_up()) return quality(current);
    maximize(alpha, shared_alpha.load(memory_order_relaxed));
    vector<Move> me = generate_moves(current, 1);
    vector<Move> enemy = generate_moves(current, 0);

    if (me.empty()) return quality(current);

    if (enemy.empty()) {
        ll best_quality = -1e18;
        for (Move after_me : me) maximize(best_quality, quality(after_me.nxt_state));
        return best_quality;
    }

    ll best_quality = -1e18;
    for (Move after_me : me){
        ll tmp = 1e18;
        for (Move after_enemy : enemy) {
            State next_2turn = apply_both_moves(current, after_me.move, after_enemy.move);
            ll nxt_quality = simulator(next_2turn, tick + 2, alpha, beta);
            minimize(tmp, nxt_quality);
            if (tmp <= alpha) break;
            if (time_is_up()) break;
        }
        if (tmp != 1e18) maximize(best_quality, tmp);
        maximize(alpha, best_quality);
        if (best_quality >= beta) break;
        if (time_is_up()) break;
    }
    return best_quality;
}

// Đơn vị công việc giao cho MỖI LUỒNG khi chia gốc: đánh giá đầy đủ 1 ứng viên nước đi
// gốc của mình bằng cách tìm phản ứng tệ nhất của địch (MIN qua mọi enemy move), y hệt
// logic nhánh "enemy không rỗng" trong simulator() ở trên nhưng KHÔNG lặp qua các ứng
// viên gốc khác. Đọc shared_alpha TƯƠI ở mỗi vòng lặp (không chỉ 1 lần lúc vào hàm) để
// bắt kịp cải thiện từ luồng khác ngay khi nó vừa xong.
ll evaluate_root_candidate(const State &current, const Move &after_me, const vector<Move> &enemy){
    if (enemy.empty()) return quality(after_me.nxt_state);
    ll tmp = 1e18;
    for (Move after_enemy : enemy) {
        ll alpha = shared_alpha.load(memory_order_relaxed);
        State next_2turn = apply_both_moves(current, after_me.move, after_enemy.move);
        ll nxt_quality = simulator(next_2turn, 3, alpha);
        minimize(tmp, nxt_quality);
        if (tmp <= alpha) break;
        if (time_is_up()) break;
    }
    // Chỉ luồng THỰC SỰ hoàn tất (không bị time_is_up cắt ngang giữa vòng for) mới có
    // tmp là giá trị CHÍNH XÁC (MIN qua đủ mọi phản ứng của địch) - trường hợp bị cắt
    // bởi alpha (tmp <= alpha) thì tmp chắc chắn không lớn hơn shared_alpha hiện tại
    // nên compare_exchange bên dưới tự động thành no-op, không cần if riêng để loại trừ.
    ll observed = shared_alpha.load(memory_order_relaxed);
    while (tmp > observed && !shared_alpha.compare_exchange_weak(observed, tmp, memory_order_relaxed)) {}
    return tmp;
}

// Phạt đảo-ngược khi so sánh ứng viên GỐC. Trước đây =10 - vô hình so với thang trọng số
// quality() (score=100000, dead_corner=5000, goal=100, approach=50, push=25): chỉ 1 đơn vị
// thay đổi ở approach/goal/push đã vượt xa mức phạt này, nên minimax không hề bị cản khi đảo
// hướng mỗi tick (xác nhận thực nghiệm: bot vẫn lặp 15xL/15xR dù đã có phạt 10). Nâng lên
// ngang tầm goal/approach*vài chục bước để thực sự cân được các thành phần đó, nhưng vẫn nhỏ
// hơn dead_corner - không được phép khiến bot đi vào góc chết hay bỏ lỡ điểm chỉ để né đảo
// hướng. Lưu ý: chỉ so last_move (1 nước ngay trước) nên KHÔNG bắt được chu trình có tick đệm
// xen giữa (vd L...D...R...D...L, tick D làm last_move không còn là hướng ngang bị đảo) - đã
// xác nhận đây chính là dạng bug thực tế gặp phải, mức tăng này giảm nhẹ chứ không triệt để.
const ll REVERSAL_PENALTY = 3000;

// Thay simulator(current, 1, 1) gọi trực tiếp: điều phối việc chia 4 ứng viên nước đi
// gốc cho các luồng, CHỜ tất cả xong (join) rồi mới đọc kết quả và ghi chosen_move ở
// luồng chính - không có luồng con nào từng ghi vào biến toàn cục trong lúc chạy, nên
// không cần mutex/lock ở đây.
void run_search(const State &current){
    chosen_move = {-1, -1};
    shared_alpha.store(-1e18, memory_order_relaxed);
    record_position(current.me);
    vector<Move> me = generate_moves(current, 1);
    vector<Move> enemy = generate_moves(current, 0);
    if (me.empty()) return;

    // Loại bớt ứng viên GỐC thuần đi bộ (không đẩy hộp) dẫn về 1 ô đã đứng trong ~20 tick gần
    // đây - chặn chu kỳ TỪ GỐC dựa trên vị trí thực tế, không phụ thuộc mức REVERSAL_PENALTY
    // (xem giải thích ở visited_recently() phía trên). Luôn giữ lại ít nhất 1 ứng viên: nếu lọc
    // hết sạch (mọi hướng đều dẫn về ô cũ, vd ngõ cụt thật) thì dùng lại danh sách đầy đủ, không
    // bao giờ để rỗng.
    vector<Move> filtered;
    for (const Move &m : me)
        if (move_pushes_box(current, m.move) || !visited_recently(m.nxt_state.me))
            filtered.push_back(m);
    if (!filtered.empty()) me = filtered;

    unsigned hardware_threads = max(1u, thread::hardware_concurrency());
    int worker_count = (int)min<size_t>(me.size(), hardware_threads);
    vector<ll> results(me.size());
    vector<thread> workers;
    workers.reserve(worker_count);

    for (int w = 0; w < worker_count; ++w) {
        workers.emplace_back([&, w]() {
            for (size_t i = w; i < me.size(); i += worker_count)
                results[i] = evaluate_root_candidate(current, me[i], enemy);
        });
    }
    for (thread &t : workers) t.join();

    ll best_quality = -1e18;
    for (size_t i = 0; i < me.size(); ++i) {
        ll candidate_quality = results[i];
        if (is_reverse_move(me[i].move, last_move)) candidate_quality -= REVERSAL_PENALTY;
        if (maximize(best_quality, candidate_quality)) chosen_move = {me[i].move, -1};
    }
}