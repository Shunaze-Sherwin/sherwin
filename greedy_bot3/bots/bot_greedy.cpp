#pragma once
#include <bits/stdc++.h>
#include "../game/rules.cpp"
#include "../search/bfs.cpp"
#include "../core/timer.cpp"
using namespace std;

const int GREEDY_DIST_CAP = 40;

// Trọng số chi phí đi bộ trong hàm chấm điểm: đủ lớn để chi phí LUÔN quyết định
// trước gain (giữ nguyên hành vi gốc: so cost trước, gain chỉ phá hòa). Tranh
// chấp đối thủ bên dưới chỉ có tác dụng phá hòa/gần-hòa giữa các ứng viên có
// chi phí xấp xỉ nhau, không bao giờ lật ngược một chênh lệch cost rõ ràng.
const int COST_WEIGHT = 100;

// Trọng số điều chỉnh theo mức độ "nóng" của cuộc tranh chấp hộp (port từ
// greedy_bot2/bot_greedy.cpp, đã kiểm chứng có lợi bằng đấu thử judge.exe).
const int CONTEST_RANGE = 8;       // đối thủ trong tầm này coi là đang thực sự nhắm hộp
const int CONTEST_WEIGHT = 2;      // độ ưu tiên cộng thêm khi tranh hộp nóng
const int LOSING_RACE_PENALTY = 1; // độ hạ ưu tiên mỗi bước khi chắc chắn thua đối thủ

// Với mỗi hộp, ước lượng tổng số bước đối thủ cần (đi tới cạnh hộp + hộp di
// chuyển tới đích của họ) để tự mình ghi điểm bằng hộp đó. Dùng bfsAvoid/
// bfsToGoals sẵn có của bot3 (né hộp khác + agent của MÌNH khi tính đường đi
// của đối thủ), không mô phỏng đẩy hộp từng bước, chỉ ước lượng khoảng cách.
vector<int> opponentThreat(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    char enemyGoal = isa ? 'B' : 'A';

    vector<vector<int>> distToEnemyGoal = bfsToGoals(state.grid, enemyGoal);

    vector<int> threat(state.boxes.size(), INF);
    for (size_t i = 0; i < state.boxes.size(); ++i) {
        Pos box = state.boxes[i];
        if (distToEnemyGoal[box.r][box.c] == INF) continue;

        Occupancy occ = emptyOccupancy();
        for (size_t j = 0; j < state.boxes.size(); ++j) {
            if (j == i) continue;
            Pos b = state.boxes[j];
            if (state.grid.checkin(b)) occ[b.r][b.c] = true;
        }
        if (state.grid.checkin(agent)) occ[agent.r][agent.c] = true;

        vector<vector<int>> oppDist = bfsAvoid(state.grid, other, occ);

        int bestApproach = INF;
        for (int k = 0; k < 4; ++k) {
            Pos standCandidate = {box.r - dr[k], box.c - dc[k]};
            if (!state.grid.isFree(standCandidate)) continue;
            if (occ[standCandidate.r][standCandidate.c]) continue;
            bestApproach = min(bestApproach, oppDist[standCandidate.r][standCandidate.c]);
        }
        if (bestApproach == INF) continue;

        threat[i] = bestApproach + distToEnemyGoal[box.r][box.c];
    }
    return threat;
}

// BOT 1 — Greedy nearest box, NHƯNG có nhắm ô đích.
// Chọn cặp (hộp, hướng đẩy) sao cho cú đẩy làm hộp TIẾN GẦN ô đích của mình,
// rồi đi tới ô đứng cần thiết để thực hiện cú đẩy đó.
// (Bản cũ chỉ đi tới hộp gần nhất rồi đẩy theo hướng BFS tuỳ ý, nên hộp bị
//  đẩy đi lung tung và gần như không bao giờ ghi được điểm.)
//
// Đã bổ sung 2 phần port từ greedy_bot2 (kiểm chứng có lợi bằng đấu thử):
//   - Nhận thức đối thủ (opponentThreat ở trên): ưu tiên tranh hộp còn kịp,
//     hạ ưu tiên hộp chắc chắn thua đối thủ trong cuộc đua.
//   - isFrozenSquare (game/rules.cpp): chặn thêm thế kẹt "khối vuông 2x2"
//     giữa hộp với hộp/tường, ngoài isBoxDeadlocked (chỉ xét 1 hộp/tường).
//
// Timer: kiểm tra istimeup() mỗi 8 lần lặp (không phải mỗi lần lặp, để
// tránh overhead gọi chrono::now() quá thường xuyên trên vòng lặp nhỏ
// numBoxes*4). Nếu hết giờ giữa chừng, dùng ngay kết quả tốt nhất đã có.
char botGreedy(const gamestate& state, bool isa, const Timer& timer) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    if (!state.grid.checkin(agent)) return 'S';
    if (state.boxes.empty()) return 'S';

    char myGoal = isa ? 'A' : 'B';

    Occupancy occ = buildOccupancy(state, isa);
    vector<vector<int>> distAgent = bfsAvoid(state.grid, agent, occ);
    vector<vector<int>> distGoal  = bfsToGoals(state.grid, myGoal);
    vector<int> threat = opponentThreat(state, isa);

    bool found = false;
    int bestScore = INF;
    Pos bestStand = {-1, -1};
    char bestDir = 'S';

    int iterCount = 0;
    bool timedOut = false;
    for (size_t bi = 0; bi < state.boxes.size() && !timedOut; ++bi) {
        Pos box = state.boxes[bi];

        // TASK DEADLOCK-01: hộp đã kẹt vĩnh viễn (isBoxDeadlocked) không còn
        // hướng đẩy khả thi nào (đã chứng minh trong rules.cpp) -- bỏ qua
        // ngay, không phí thời gian duyệt 4 hướng cho hộp này (mọi hướng đều
        // sẽ fail điều kiện dest/stand free bên dưới, nhưng skip sớm cho rõ
        // ý và tiết kiệm 1 vòng lặp con).
        if (isBoxDeadlocked(state.grid, box)) continue;

        vector<Pos> otherBoxes;
        for (size_t j = 0; j < state.boxes.size(); ++j) if (j != bi) otherBoxes.push_back(state.boxes[j]);

        for (int i = 0; i < 4; ++i) {
            if ((++iterCount % 8) == 0 && timer.istimeup()) { timedOut = true; break; }
            Pos dest  = {box.r + dr[i], box.c + dc[i]};   // hộp sẽ tới đây
            Pos stand = {box.r - dr[i], box.c - dc[i]};   // mình phải đứng đây để đẩy

            // Ô hộp sẽ tới phải trống và không có hộp/người khác
            if (!state.grid.isFree(dest)) continue;
            if (state.hasBox(dest)) continue;
            if (state.grid.checkin(other) && dest == other) continue;

            // Ô mình phải đứng cũng vậy (trừ khi mình đang đứng sẵn ở đó)
            if (!state.grid.isFree(stand)) continue;
            if (state.hasBox(stand)) continue;
            if (state.grid.checkin(other) && stand == other) continue;

            // TASK DEADLOCK-01: không chủ động đẩy hộp vào 1 ô sẽ khiến nó
            // kẹt vĩnh viễn ngay sau cú đẩy này, dù cú đẩy đó có "gain" dương
            // theo khoảng cách tới đích (gần đích hơn về mặt BFS nhưng lại
            // biến hộp thành vô dụng vĩnh viễn -- gain ngắn hạn không đáng).
            //
            // NGOẠI LỆ QUAN TRỌNG: nếu "dest" chính là ô đích ('A' hoặc 'B'),
            // hộp SẼ GHI ĐIỂM VÀ BIẾN MẤT NGAY (applyMove/rules.cpp) -- không
            // bao giờ thực sự "nằm kẹt" ở đó. isBoxDeadlocked() chỉ nhìn
            // tường xung quanh, không biết về ký tự ô đích, nên sẽ báo sai
            // (dương tính giả) cho MỌI ô đích tình cờ nằm cạnh 2 tường (ví dụ
            // đích ở góc bản đồ -- rất bình thường trong map_generator).
            // Đã phát hiện qua A/B thật (docs/eval_experiments.md Vòng 6):
            // thiếu ngoại lệ này làm giảm hẳn điểm trung bình vì bot từ chối
            // cả những cú đẩy ghi điểm hợp lệ.
            char destCell = state.grid.cell[dest.r][dest.c];
            bool destIsGoal = (destCell == 'A' || destCell == 'B');
            if (!destIsGoal && isBoxDeadlocked(state.grid, dest)) continue;
            if (!destIsGoal && isFrozenSquare(state.grid, dest, otherBoxes)) continue;

            int dBox  = min(distGoal[box.r][box.c], GREEDY_DIST_CAP);
            int dDest = min(distGoal[dest.r][dest.c], GREEDY_DIST_CAP);
            int gain  = dBox - dDest;
            if (gain <= 0) continue;   // cú đẩy này không đưa hộp lại gần đích

            int cost;
            if (agent == stand) cost = 0;
            else cost = distAgent[stand.r][stand.c];
            if (cost >= INF) continue;

            // Chi phí đi bộ quyết định trước, gain chỉ phá hòa (như hành vi gốc).
            int score = cost * COST_WEIGHT - gain;

            // Điều chỉnh theo mức độ tranh chấp với đối thủ (phá hòa/gần-hòa).
            int myTotal = dDest + cost;
            int oppTotal = threat[bi];
            if (oppTotal != INF) {
                if (myTotal <= oppTotal) {
                    int urgency = max(0, CONTEST_RANGE - oppTotal);
                    score -= urgency * CONTEST_WEIGHT;
                } else {
                    score += (myTotal - oppTotal) * LOSING_RACE_PENALTY;
                }
            }

            if (!found || score < bestScore) {
                found = true;
                bestScore = score;
                bestStand = stand;
                bestDir = DIRCH[i];
            }
        }
    }

    char move = 'S';

    if (found) {
        if (agent == bestStand) move = bestDir;                              // đẩy luôn
        else move = firstStepTowards(state.grid, agent, bestStand, occ);     // đi tới chỗ đứng
    }

    // Fallback: không có cú đẩy nào có lợi -> tiến về ô cạnh hộp gần nhất
    // (TASK DEADLOCK-01: bỏ qua hộp đã kẹt vĩnh viễn -- đi tới cạnh 1 hộp
    // không ai đẩy được nữa là lãng phí lượt, và chính là nguyên nhân quan
    // sát được của hiện tượng "Stuck%" cao trong benchmark, xem
    // docs/eval_experiments.md Vòng 5).
    if (move == 'S') {
        int bestD = INF;
        Pos target = {-1, -1};
        for (const Pos& box : state.boxes) {
            if (isBoxDeadlocked(state.grid, box)) continue;
            for (int i = 0; i < 4; ++i) {
                Pos nb = {box.r + dr[i], box.c + dc[i]};
                if (!state.grid.isFree(nb) || state.hasBox(nb)) continue;
                if (nb == agent) continue;
                int d = distAgent[nb.r][nb.c];
                if (d < bestD) { bestD = d; target = nb; }
            }
        }
        if (target.r != -1 && bestD < INF) {
            move = firstStepTowards(state.grid, agent, target, occ);
        }
    }

    // Chốt an toàn: không bao giờ trả về nước đi bất hợp lệ
    if (!canpush(state, agent, move)) return 'S';
    return move;
}

// Bản tương thích ngược: không truyền Timer -> coi như không giới hạn thời gian.
char botGreedy(const gamestate& state, bool isa) {
    Timer unlimited(1e9);
    return botGreedy(state, isa, unlimited);
}
