#pragma once
#include <bits/stdc++.h>
#include "../game/rules.cpp"
#include "../search/bfs.cpp"
#include "../core/timer.cpp"
using namespace std;

const int GREEDY_DIST_CAP = 40;

// BOT 1 — Greedy nearest box, NHƯNG có nhắm ô đích.
// Chọn cặp (hộp, hướng đẩy) sao cho cú đẩy làm hộp TIẾN GẦN ô đích của mình,
// rồi đi tới ô đứng cần thiết để thực hiện cú đẩy đó.
// (Bản cũ chỉ đi tới hộp gần nhất rồi đẩy theo hướng BFS tuỳ ý, nên hộp bị
//  đẩy đi lung tung và gần như không bao giờ ghi được điểm.)
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

    bool found = false;
    int bestCost = INF, bestGain = -1;
    Pos bestStand = {-1, -1};
    char bestDir = 'S';

    int iterCount = 0;
    bool timedOut = false;
    for (const Pos& box : state.boxes) {
        if (timedOut) break;

        // TASK DEADLOCK-01: hộp đã kẹt vĩnh viễn (isBoxDeadlocked) không còn
        // hướng đẩy khả thi nào (đã chứng minh trong rules.cpp) -- bỏ qua
        // ngay, không phí thời gian duyệt 4 hướng cho hộp này (mọi hướng đều
        // sẽ fail điều kiện dest/stand free bên dưới, nhưng skip sớm cho rõ
        // ý và tiết kiệm 1 vòng lặp con).
        if (isBoxDeadlocked(state.grid, box)) continue;

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

            int dBox  = min(distGoal[box.r][box.c], GREEDY_DIST_CAP);
            int dDest = min(distGoal[dest.r][dest.c], GREEDY_DIST_CAP);
            int gain  = dBox - dDest;
            if (gain <= 0) continue;   // cú đẩy này không đưa hộp lại gần đích

            int cost;
            if (agent == stand) cost = 0;
            else cost = distAgent[stand.r][stand.c];
            if (cost >= INF) continue;

            if (!found || cost < bestCost || (cost == bestCost && gain > bestGain)) {
                found = true;
                bestCost = cost;
                bestGain = gain;
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
