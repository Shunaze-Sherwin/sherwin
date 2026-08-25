#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "bfs.cpp"
#include "timer.cpp"
using namespace std;

// upgrade.h — bản botGreedy có 2 cải tiến port từ greedy_bot2/bot_greedy.cpp //upgrade
// (đã kiểm chứng có lợi bằng đấu thử judge.exe), tách riêng khỏi bot_greedy.cpp //upgrade
// gốc để giữ nguyên file gốc không đổi. Đây là bản THAY THẾ (cùng tên hàm //upgrade
// botGreedy) — muốn dùng bản này thay vì bản gốc thì đổi include trong //upgrade
// main.cpp từ "bot_greedy.cpp" sang "upgrade.h" (KHÔNG include cả 2 cùng lúc //upgrade
// trong 1 file .cpp, sẽ bị lỗi định nghĩa trùng tên). //upgrade
// //upgrade
// Kết quả đấu thử (greedy_bot3 dùng bản upgrade này) trong repo sherwin: //upgrade
//   vs greedy_bot2:        200 ván: 52-87-61 | 400 ván: 125-179-96 | 1000 ván: 324-408-268 //upgrade
//                           (greedy_bot2 thắng đều ~55-57% số ván phân thắng bại) //upgrade
//   vs greedy_bot (gốc):   200 ván: 83 thắng - 74 hòa - 43 thua //upgrade
//   vs heuristic_bot:      200 ván: 153 thắng - 46 hòa - 1 thua //upgrade
//   vs custom (Q-learning): 10 ván: 9 thắng - 0 hòa - 1 thua //upgrade

// KHONG DUNG LAM BOT NOP BAI. Chua qua A/B noi bo bang chinh
// bench_bot_winrate.cpp/tests/ cua project nay -- so lieu duoi day la tu
// mot judge/repo khac, khong du de tin theo dung quy tac Muc K/L. Xem
// docs/eval_experiments.md Vong 8.
//
// TASK STEP-05 (roadmap.md): phan isFrozenSquare da duoc port sang
// src/bots/bot_greedy.cpp (ban chinh thuc, da A/B: winrate 82-84%, diem TB
// 1.92->2.0-2.1, 0 loi sanitizer/1000+ van -- xem docs/eval_experiments.md
// Vong 8) va DA XOA khoi file nay de tranh code trung lap. Chi con lai
// opponentThreat ben duoi, giu lam tai lieu tham khao cho y tuong "Cong 4 --
// doi khang that" neu sau nay co thoi gian -- KHONG phai hom nay.

const int GREEDY_DIST_CAP = 40;

// Trọng số chi phí đi bộ trong hàm chấm điểm: đủ lớn để chi phí LUÔN quyết định //upgrade
// trước gain (giữ nguyên hành vi gốc: so cost trước, gain chỉ phá hòa). Tranh //upgrade
// chấp đối thủ bên dưới chỉ có tác dụng phá hòa/gần-hòa giữa các ứng viên có //upgrade
// chi phí xấp xỉ nhau, không bao giờ lật ngược một chênh lệch cost rõ ràng. //upgrade
const int COST_WEIGHT = 100; //upgrade

// Trọng số điều chỉnh theo mức độ "nóng" của cuộc tranh chấp hộp (port từ //upgrade
// greedy_bot2/bot_greedy.cpp, đã kiểm chứng có lợi bằng đấu thử judge.exe). //upgrade
const int CONTEST_RANGE = 8;       // đối thủ trong tầm này coi là đang thực sự nhắm hộp //upgrade
const int CONTEST_WEIGHT = 2;      // độ ưu tiên cộng thêm khi tranh hộp nóng //upgrade
const int LOSING_RACE_PENALTY = 1; // độ hạ ưu tiên mỗi bước khi chắc chắn thua đối thủ //upgrade

// isFrozenSquare — mở rộng isBoxDeadlocked (rules.cpp) cho thế kẹt "khối
// vuông 2x2" giữa HỘP VỚI HỘP (isBoxDeadlocked chỉ xét tường quanh 1 hộp,
// không biết hộp khác cũng có thể đóng vai trò như tường tạm thời).
//
// Đồng bộ lại từ greedy_bot3/bots/upgrade.h - bản trước đó của file này ghi
// "đã chuyển sang src/bots/bot_greedy.cpp" (đường dẫn không tồn tại trong
// repo này) và không tự định nghĩa lại, khiến file KHÔNG compile được độc
// lập (thiếu symbol isFrozenSquare ở dòng gọi bên dưới) - đã xác nhận qua
// build thật khi biên dịch greedy_bot4 lần đầu.
inline bool isFrozenSquare(const Grid& g, Pos p, const vector<Pos>& otherBoxes) { //upgrade
    if (g.cell[p.r][p.c] == 'A' || g.cell[p.r][p.c] == 'B') return false; //upgrade

    auto occupied = [&](Pos q) { //upgrade
        if (g.isWall(q)) return true; //upgrade
        for (const Pos& b : otherBoxes) if (b == q) return true; //upgrade
        return false; //upgrade
    }; //upgrade

    const int quadR[] = {-1, -1, 1, 1}; //upgrade
    const int quadC[] = {-1, 1, -1, 1}; //upgrade
    for (int k = 0; k < 4; ++k) { //upgrade
        Pos horiz = {p.r, p.c + quadC[k]}; //upgrade
        Pos vert  = {p.r + quadR[k], p.c}; //upgrade
        Pos diag  = {p.r + quadR[k], p.c + quadC[k]}; //upgrade
        if (occupied(horiz) && occupied(vert) && occupied(diag)) return true; //upgrade
    } //upgrade
    return false; //upgrade
} //upgrade

// Với mỗi hộp, ước lượng tổng số bước đối thủ cần (đi tới cạnh hộp + hộp di //upgrade
// chuyển tới đích của họ) để tự mình ghi điểm bằng hộp đó. Dùng bfsAvoid/ //upgrade
// bfsToGoals sẵn có của bot3 (né hộp khác + agent của MÌNH khi tính đường đi //upgrade
// của đối thủ), không mô phỏng đẩy hộp từng bước, chỉ ước lượng khoảng cách. //upgrade
vector<int> opponentThreat(const gamestate& state, bool isa) { //upgrade
    Pos agent = isa ? state.a : state.b; //upgrade
    Pos other = isa ? state.b : state.a; //upgrade
    char enemyGoal = isa ? 'B' : 'A'; //upgrade

    vector<vector<int>> distToEnemyGoal = bfsToGoals(state.grid, enemyGoal); //upgrade

    vector<int> threat(state.boxes.size(), INF); //upgrade
    for (size_t i = 0; i < state.boxes.size(); ++i) { //upgrade
        Pos box = state.boxes[i]; //upgrade
        if (distToEnemyGoal[box.r][box.c] == INF) continue; //upgrade

        Occupancy occ = emptyOccupancy(); //upgrade
        for (size_t j = 0; j < state.boxes.size(); ++j) { //upgrade
            if (j == i) continue; //upgrade
            Pos b = state.boxes[j]; //upgrade
            if (state.grid.checkin(b)) occ[b.r][b.c] = true; //upgrade
        } //upgrade
        if (state.grid.checkin(agent)) occ[agent.r][agent.c] = true; //upgrade

        vector<vector<int>> oppDist = bfsAvoid(state.grid, other, occ); //upgrade

        int bestApproach = INF; //upgrade
        for (int k = 0; k < 4; ++k) { //upgrade
            Pos standCandidate = {box.r - dr[k], box.c - dc[k]}; //upgrade
            if (!state.grid.isFree(standCandidate)) continue; //upgrade
            if (occ[standCandidate.r][standCandidate.c]) continue; //upgrade
            bestApproach = min(bestApproach, oppDist[standCandidate.r][standCandidate.c]); //upgrade
        } //upgrade
        if (bestApproach == INF) continue; //upgrade

        threat[i] = bestApproach + distToEnemyGoal[box.r][box.c]; //upgrade
    } //upgrade
    return threat; //upgrade
} //upgrade

// BOT 1 — Greedy nearest box, NHƯNG có nhắm ô đích.
// Chọn cặp (hộp, hướng đẩy) sao cho cú đẩy làm hộp TIẾN GẦN ô đích của mình,
// rồi đi tới ô đứng cần thiết để thực hiện cú đẩy đó.
// (Bản cũ chỉ đi tới hộp gần nhất rồi đẩy theo hướng BFS tuỳ ý, nên hộp bị
//  đẩy đi lung tung và gần như không bao giờ ghi được điểm.)
//
// Đã bổ sung 2 phần port từ greedy_bot2 (kiểm chứng có lợi bằng đấu thử): //upgrade
//   - Nhận thức đối thủ (opponentThreat ở trên): ưu tiên tranh hộp còn kịp, //upgrade
//     hạ ưu tiên hộp chắc chắn thua đối thủ trong cuộc đua. //upgrade
//   - isFrozenSquare ở trên: chặn thêm thế kẹt "khối vuông 2x2" giữa hộp với //upgrade
//     hộp/tường, ngoài isBoxDeadlocked (chỉ xét 1 hộp/tường). //upgrade
// //upgrade
// CHƯA đổi: fallback khi hết nước đẩy có lợi vẫn là "tiến tới hộp gần nhất" //upgrade
// như bản gốc (KHÔNG chuyển sang kiểu "chặn đường đối thủ" như greedy_bot2) — //upgrade
// đây là giả thuyết đang mở, chưa kiểm chứng, về nguyên nhân greedy_bot2 vẫn //upgrade
// thắng nhỉnh hơn bản upgrade này (~55-57% số ván phân thắng bại). //upgrade
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
    vector<int> threat = opponentThreat(state, isa); //upgrade

    bool found = false;
    int bestScore = INF; //upgrade
    Pos bestStand = {-1, -1};
    char bestDir = 'S';

    int iterCount = 0;
    bool timedOut = false;
    for (size_t bi = 0; bi < state.boxes.size() && !timedOut; ++bi) { //upgrade
        Pos box = state.boxes[bi]; //upgrade

        // TASK DEADLOCK-01: hộp đã kẹt vĩnh viễn (isBoxDeadlocked) không còn
        // hướng đẩy khả thi nào (đã chứng minh trong rules.cpp) -- bỏ qua
        // ngay, không phí thời gian duyệt 4 hướng cho hộp này (mọi hướng đều
        // sẽ fail điều kiện dest/stand free bên dưới, nhưng skip sớm cho rõ
        // ý và tiết kiệm 1 vòng lặp con).
        if (isBoxDeadlocked(state.grid, box)) continue;

        vector<Pos> otherBoxes; //upgrade
        for (size_t j = 0; j < state.boxes.size(); ++j) if (j != bi) otherBoxes.push_back(state.boxes[j]); //upgrade

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
            // LUU Y (STEP-05): dong nay tham chieu isFrozenSquare, nhung dinh
            // nghia cua no da chuyen han sang bot_greedy.cpp -- file nay
            // KHONG con tu compile duoc rieng le nua. Chap nhan duoc vi file
            // nay khong nam trong bat ky build target nao (main.cpp,
            // benchmarks/, tests/ deu khong include no) -- thuan tuy tai
            // lieu tham khao cho opponentThreat. Neu muon compile lai, them
            // #include "bot_greedy.cpp" va doi ten ham botGreedy() ben duoi
            // de tranh dung ten voi ban chinh thuc.
            if (!destIsGoal && isFrozenSquare(state.grid, dest, otherBoxes)) continue; //upgrade

            int dBox  = min(distGoal[box.r][box.c], GREEDY_DIST_CAP);
            int dDest = min(distGoal[dest.r][dest.c], GREEDY_DIST_CAP);
            int gain  = dBox - dDest;
            if (gain <= 0) continue;   // cú đẩy này không đưa hộp lại gần đích

            int cost;
            if (agent == stand) cost = 0;
            else cost = distAgent[stand.r][stand.c];
            if (cost >= INF) continue;

            // Chi phí đi bộ quyết định trước, gain chỉ phá hòa (như hành vi gốc). //upgrade
            int score = cost * COST_WEIGHT - gain; //upgrade

            // Điều chỉnh theo mức độ tranh chấp với đối thủ (phá hòa/gần-hòa). //upgrade
            int myTotal = dDest + cost; //upgrade
            int oppTotal = threat[bi]; //upgrade
            if (oppTotal != INF) { //upgrade
                if (myTotal <= oppTotal) { //upgrade
                    int urgency = max(0, CONTEST_RANGE - oppTotal); //upgrade
                    score -= urgency * CONTEST_WEIGHT; //upgrade
                } else { //upgrade
                    score += (myTotal - oppTotal) * LOSING_RACE_PENALTY; //upgrade
                } //upgrade
            } //upgrade

            if (!found || score < bestScore) { //upgrade
                found = true;
                bestScore = score; //upgrade
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
