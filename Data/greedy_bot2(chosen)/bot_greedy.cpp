#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "bfs.cpp"
#include "timer.cpp"
using namespace std;

// Bot Greedy2: kết hợp 3 phần đã kiểm chứng bằng đấu thử (judge.exe) là thực sự tốt
// hơn Bot Greedy gốc và/hoặc bản Greedy2 trước đó của repo này:
//
//   1. Chọn nước đi kiểu "chi phí trước" (học từ bot3, xem greedy_bot3/): trong các
//      cú đẩy THỰC SỰ đưa hộp gần đích hơn (gain > 0), ưu tiên cú đẩy nào đi bộ tới
//      ít bước nhất trước, tiến độ hộp (gain) chỉ là tiêu chí phụ để phá hòa. Đấu thử
//      200 ván cho thấy chiến lược này (bot3) thắng bản Greedy2 cũ (ưu tiên tiến độ
//      hộp trước) với tỉ lệ ~70% số ván phân thắng bại (85 thắng - 78 hòa - 37 thua).
//   2. Nhận thức đối thủ (đã có từ bản trước): ước lượng đối thủ cần bao nhiêu bước để
//      "cướp" mỗi hộp về đích của họ, ưu tiên tranh hộp còn kịp, hạ ưu tiên hộp chắc
//      chắn thua, và khi không còn cú đẩy nào có lợi thì chủ động chặn đường đối thủ.
//   3. Phát hiện deadlock rộng hơn (đã có từ bản trước): ngoài góc tường (isDeadCorner),
//      còn chặn cả thế kẹt "khối vuông 2x2" giữa hộp với tường/hộp khác (isFrozenSquare).
//   4. Timer an toàn (học từ bot3, timer.cpp): giới hạn thời gian tính mỗi nước, kiểm
//      tra istimeup() mỗi 8 lần lặp trong vòng quét hộp/hướng, dùng ngay kết quả tốt
//      nhất đã có nếu hết giờ giữa chừng. judge.exe cho tối đa 2000ms/nước
//      (bot_process_win.cpp:280); ngân sách đặt 1500ms để chừa margin an toàn. Bản
//      Greedy2 trước đó KHÔNG có cơ chế này, có rủi ro timeout trên bàn phức tạp hơn
//      dù chưa từng xảy ra trong các bàn 16x16 đã test.
//
// Đã thử thêm lookahead 2 nước ở một bản trước đó (mô phỏng top ứng viên rồi cộng dồn
// giá trị nước tiếp theo) nhưng đo được là làm bot YẾU hơn (3 thắng - 9 hòa - 8 thua vs
// bản gốc), vì mô phỏng giả định đối thủ đứng yên trong lúc ta đi 2 nước — sai với thực
// tế đối thủ cũng di chuyển mỗi lượt. KHÔNG đưa lookahead vào bản này.

// Trọng số điều chỉnh theo mức độ "nóng" của cuộc tranh chấp hộp.
const int CONTEST_RANGE = 8;       // đối thủ trong tầm này coi là đang thực sự nhắm hộp
const int CONTEST_WEIGHT = 2;      // độ ưu tiên cộng thêm khi tranh hộp nóng
const int LOSING_RACE_PENALTY = 1; // độ hạ ưu tiên mỗi bước khi chắc chắn thua đối thủ

// Trọng số chi phí đi bộ trong hàm chấm điểm: đặt đủ lớn để chi phí đi bộ LUÔN quyết
// định trước (giống bot3: so cost trước, gain chỉ phá hòa), điều chỉnh tranh chấp ở
// trên chỉ có tác dụng phá hòa/gần-hòa giữa các ứng viên có chi phí xấp xỉ nhau.
const int APPROACH_WEIGHT = 100;

// Ngân sách thời gian tính mỗi nước (ms). judge.exe cho tối đa 2000ms (xem
// bot_process_win.cpp:280), chừa margin an toàn.
const double TIME_BUDGET_MS = 1500.0;

// Với mỗi hộp, ước lượng tổng số bước đối thủ cần (đi tới cạnh hộp + hộp di chuyển
// tới đích của họ) để tự mình ghi điểm bằng hộp đó. Dùng BFS trên lưới mở, cùng độ
// gần đúng như cách bot tự đánh giá nước đi của chính mình (không mô phỏng đẩy hộp
// từng bước, chỉ ước lượng khoảng cách).
vector<int> opponentThreat(const gamestate& state, Pos other, Pos agent, char enemyGoal) {
    vector<vector<int>> distToEnemyGoal = bfsToGoal(state.grid, enemyGoal);

    vector<int> threat(state.boxes.size(), INF);
    for (size_t i = 0; i < state.boxes.size(); ++i) {
        Pos box = state.boxes[i];
        if (distToEnemyGoal[box.r][box.c] == INF) continue;

        vector<Pos> blocked;
        for (size_t j = 0; j < state.boxes.size(); ++j) if (j != i) blocked.push_back(state.boxes[j]);
        blocked.push_back(agent);

        vector<vector<int>> oppDist = bfsFrom(state.grid, other, blocked);

        int bestApproach = INF;
        for (int k = 0; k < 4; ++k) {
            Pos standCandidate = {box.r - dr[k], box.c - dc[k]};
            if (!state.grid.isFree(standCandidate)) continue;
            if (isBlocked(blocked, standCandidate)) continue;
            bestApproach = min(bestApproach, oppDist[standCandidate.r][standCandidate.c]);
        }
        if (bestApproach == INF) continue;

        threat[i] = bestApproach + distToEnemyGoal[box.r][box.c];
    }
    return threat;
}

struct BestMove {
    bool found = false;
    Pos standPos = {-1, -1};
    char dir = 'S';
    int score = INF;
};

// Tìm cú đẩy (hộp, hướng) tốt nhất cho một bên (isa = true tương ứng agent 'a'/đích 'A'),
// ưu tiên chi phí đi bộ trước rồi mới tới tiến độ hộp/tranh chấp đối thủ. Dùng chung cho
// cả việc tính nước đi của chính ta lẫn việc "đoán" nước đi tốt nhất của đối thủ để phục
// vụ chặn đường. Tôn trọng ngân sách thời gian của timer, trả về kết quả tốt nhất đã có
// nếu hết giờ giữa chừng thay vì quét hết toàn bộ.
BestMove findBestMove(const gamestate& state, bool isa, const Timer& timer) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    char myGoal = isa ? 'A' : 'B';
    char enemyGoal = isa ? 'B' : 'A';

    vector<vector<int>> distToMyGoal = bfsToGoal(state.grid, myGoal);
    vector<int> threat = opponentThreat(state, other, agent, enemyGoal);
    const char dirs[] = {'U', 'D', 'L', 'R'};

    BestMove best;
    int iterCount = 0;
    bool timedOut = false;

    for (size_t i = 0; i < state.boxes.size() && !timedOut; ++i) {
        Pos box = state.boxes[i];
        vector<Pos> otherBoxes;
        for (size_t j = 0; j < state.boxes.size(); ++j) if (j != i) otherBoxes.push_back(state.boxes[j]);

        for (char dir : dirs) {
            if ((++iterCount % 8) == 0 && timer.istimeup()) { timedOut = true; break; }

            auto [drv, dcv] = getidx(dir);
            Pos pushDest = {box.r + drv, box.c + dcv};
            Pos standPos = {box.r - drv, box.c - dcv};

            if (state.grid.isWall(pushDest) || state.grid.isWall(standPos)) continue;
            if (state.grid.cell[pushDest.r][pushDest.c] == enemyGoal) continue; // không tự đẩy hộp vào đích địch
            if (pushDest == other || standPos == other) continue;              // tránh vị trí bị agent địch chiếm
            if (isBlocked(otherBoxes, pushDest) || isBlocked(otherBoxes, standPos)) continue;
            if (distToMyGoal[pushDest.r][pushDest.c] == INF) continue;
            if (isDeadCorner(state.grid, pushDest)) continue;                  // tránh tự kẹt hộp vào góc chết
            if (isFrozenSquare(state.grid, pushDest, otherBoxes)) continue;    // tránh tự kẹt khối vuông 2x2

            // Chỉ xét cú đẩy THỰC SỰ đưa hộp gần đích hơn (gain > 0), như bot3.
            int gain = distToMyGoal[box.r][box.c] - distToMyGoal[pushDest.r][pushDest.c];
            if (gain <= 0) continue;

            vector<Pos> blockedForWalk = otherBoxes;
            blockedForWalk.push_back(other);
            int approachDist = bfsFrom(state.grid, agent, blockedForWalk)[standPos.r][standPos.c];
            if (approachDist == INF) continue;

            // Chi phí đi bộ quyết định trước, gain chỉ phá hòa (xem APPROACH_WEIGHT).
            int score = approachDist * APPROACH_WEIGHT - gain;

            // Điều chỉnh theo mức độ tranh chấp với đối thủ (phá hòa/gần-hòa).
            int myTotal = distToMyGoal[pushDest.r][pushDest.c] + approachDist;
            int oppTotal = threat[i];
            if (oppTotal != INF) {
                if (myTotal <= oppTotal) {
                    // Ta vẫn kịp tranh hộp này trước đối thủ: hộp càng "nóng" (đối thủ
                    // càng gần lấy được) thì càng nên ưu tiên giành trước.
                    int urgency = max(0, CONTEST_RANGE - oppTotal);
                    score -= urgency * CONTEST_WEIGHT;
                } else {
                    // Đối thủ chắc chắn tới trước: hạ ưu tiên, tránh phí lượt đuổi theo
                    // một hộp gần như chắc chắn sẽ bị họ lấy mất.
                    score += (myTotal - oppTotal) * LOSING_RACE_PENALTY;
                }
            }

            if (score < best.score) {
                best.score = score;
                best.standPos = standPos;
                best.dir = dir;
                best.found = true;
            }
        }
    }
    return best;
}

char botGreedy(const gamestate& state, bool isa, const Timer& timer) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;

    BestMove mine = findBestMove(state, isa, timer);

    if (mine.found) {
        if (agent == mine.standPos) {
            return canpush(state, agent, mine.dir) ? mine.dir : 'S';
        }
        vector<Pos> blockedForWalk = state.boxes;
        blockedForWalk.push_back(other);
        char step = find(state.grid, agent, mine.standPos, blockedForWalk);
        return canpush(state, agent, step) ? step : 'S';
    }

    // Không còn cú đẩy nào có lợi cho ta: tranh thủ tiến về ô đứng mà đối thủ cần
    // để thực hiện nước đẩy tốt nhất của họ, để chặn đường thay vì đứng yên lãng phí lượt.
    if (!timer.istimeup()) {
        BestMove theirs = findBestMove(state, !isa, timer);
        if (theirs.found) {
            vector<Pos> blockedForWalk = state.boxes;
            blockedForWalk.push_back(other);
            char step = find(state.grid, agent, theirs.standPos, blockedForWalk);
            if (step != 'S' && canpush(state, agent, step)) return step;
        }
    }

    return 'S';
}

int main(int argc, char **argv) {
    if (argc < 2 || string(argv[1]) != "--interactive") return 1;

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        vector<string> lines(16);
        for (string &line : lines) {
            if (!(cin >> line)) return 0;
        }

        gamestate state;
        state.grid = inp(lines);
        for (int row = 0; row < 16; ++row) {
            for (int column = 0; column < 16; ++column) {
                char &cell = state.grid.cell[row][column];
                if (cell == 'a') {
                    state.a = {row, column};
                    cell = '.';
                } else if (cell == 'b') {
                    state.b = {row, column};
                    cell = '.';
                } else if (cell == 'X') {
                    state.boxes.push_back({row, column});
                    cell = '.';
                }
            }
        }

        if (!(cin >> state.scoreA >> state.scoreB)) return 0;
        Timer timer(TIME_BUDGET_MS);
        cout << botGreedy(state, true, timer) << '\n';
        cout.flush();
    }
}
