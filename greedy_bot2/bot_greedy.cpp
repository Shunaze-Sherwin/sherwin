#pragma once
#include <bits/stdc++.h>
#include "rules.cpp"
#include "bfs.cpp"
using namespace std;

// Bot Greedy2: giống Bot Greedy gốc (xét mọi cặp hộp/hướng đẩy, chọn cú đẩy đưa hộp
// tới gần đích của MÌNH nhất) nhưng có thêm 2 cải tiến đã kiểm chứng bằng đấu thử
// (judge.exe, hàng chục ván) là thực sự tốt hơn bản gốc:
//   1. Nhận thức đối thủ: ước lượng đối thủ cần bao nhiêu bước để "cướp" mỗi hộp về
//      đích của họ, ưu tiên tranh hộp còn kịp, hạ ưu tiên hộp chắc chắn thua, và khi
//      không còn cú đẩy nào có lợi thì chủ động tiến về chặn đường đối thủ.
//   2. Phát hiện deadlock rộng hơn: ngoài góc tường (isDeadCorner), còn chặn cả thế
//      kẹt "khối vuông 2x2" giữa hộp với tường/hộp khác (isFrozenSquare).
// Kết quả 30 ván vs bot Greedy gốc: 11 thắng - 15 hòa - 4 thua.
//
// Đã thử thêm lookahead 2 nước (mô phỏng top ứng viên rồi cộng dồn giá trị nước tiếp
// theo) nhưng đo được là làm bot YẾU hơn (3 thắng - 9 hòa - 8 thua vs bản gốc), vì mô
// phỏng giả định đối thủ đứng yên trong lúc ta đi 2 nước — sai với thực tế đối thủ
// cũng di chuyển mỗi lượt, khiến bot đánh giá sai giá trị tương lai. Vì vậy KHÔNG đưa
// lookahead vào bản chính thức này; nếu muốn thử lại, cần mô phỏng cả nước đi của đối
// thủ (không chỉ đứng yên) thì mới có cơ sở để đánh giá đúng.

// Trọng số ưu tiên tiến độ hộp so với quãng đường phải đi bộ trong hàm chấm điểm gốc.
const int PROGRESS_WEIGHT = 4;

// Trọng số điều chỉnh theo mức độ "nóng" của cuộc tranh chấp hộp. Chỉnh các hằng số
// này nếu muốn bot tranh chấp quyết liệt hơn/ít hơn.
const int CONTEST_RANGE = 8;       // đối thủ trong tầm này coi là đang thực sự nhắm hộp
const int CONTEST_WEIGHT = 2;      // độ ưu tiên cộng thêm khi tranh hộp nóng
const int LOSING_RACE_PENALTY = 1; // độ hạ ưu tiên mỗi bước khi chắc chắn thua đối thủ

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
// có tính thêm mức độ tranh chấp với đối thủ. Dùng chung cho cả việc tính nước đi của
// chính ta lẫn việc "đoán" nước đi tốt nhất của đối thủ để phục vụ chặn đường.
BestMove findBestMove(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    char myGoal = isa ? 'A' : 'B';
    char enemyGoal = isa ? 'B' : 'A';

    vector<vector<int>> distToMyGoal = bfsToGoal(state.grid, myGoal);
    vector<int> threat = opponentThreat(state, other, agent, enemyGoal);
    const char dirs[] = {'U', 'D', 'L', 'R'};

    BestMove best;

    for (size_t i = 0; i < state.boxes.size(); ++i) {
        Pos box = state.boxes[i];
        vector<Pos> otherBoxes;
        for (size_t j = 0; j < state.boxes.size(); ++j) if (j != i) otherBoxes.push_back(state.boxes[j]);

        for (char dir : dirs) {
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

            vector<Pos> blockedForWalk = otherBoxes;
            blockedForWalk.push_back(other);
            int approachDist = bfsFrom(state.grid, agent, blockedForWalk)[standPos.r][standPos.c];
            if (approachDist == INF) continue;

            // Ưu tiên tiến độ hộp tới đích hơn quãng đường phải đi bộ (như bot gốc).
            int score = distToMyGoal[pushDest.r][pushDest.c] * PROGRESS_WEIGHT + approachDist;

            // Điều chỉnh theo mức độ tranh chấp với đối thủ.
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

char botGreedy(const gamestate& state, bool isa) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;

    BestMove mine = findBestMove(state, isa);

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
    BestMove theirs = findBestMove(state, !isa);
    if (theirs.found) {
        vector<Pos> blockedForWalk = state.boxes;
        blockedForWalk.push_back(other);
        char step = find(state.grid, agent, theirs.standPos, blockedForWalk);
        if (step != 'S' && canpush(state, agent, step)) return step;
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
        cout << botGreedy(state, true) << '\n';
        cout.flush();
    }
}
