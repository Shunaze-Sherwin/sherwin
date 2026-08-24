#pragma once
#include <bits/stdc++.h>
#include "state.cpp"
using namespace std;

// Chuyển đổi ký tự hướng đi thành mức độ thay đổi tọa độ
pair<int, int> getidx(char dir) {
    if (dir == 'U') return {-1, 0};
    if (dir == 'D') return {1, 0};
    if (dir == 'L') return {0, -1};
    if (dir == 'R') return {0, 1};
    return {0, 0}; // 'S' - Đứng yên
}

// ========================================================
// CONFIGURABLE RULES (Sẽ sửa nếu đề thi thật có luật khác)
//
// Mỗi điểm biến thiên đã liệt kê trong roadmap.md (TASK IO-03) được đánh số
// // CONFIGURABLE-N ngay tại dòng liên quan, để khi đọc đề thật có thể tra
// nhanh qua docs/decision_tree.md thay vì dò lại toàn bộ hàm.
//
//   CONFIGURABLE-1: đẩy được bao nhiêu ô 1 lần         (hiện: đúng 1 ô)
//   CONFIGURABLE-2: đẩy được bao nhiêu hộp 1 lần        (hiện: đúng 1 hộp)
//   CONFIGURABLE-3: đi vòng qua ô có hộp không đẩy?     (hiện: KHÔNG — mọi
//                   lần đi vào ô có hộp đều được hiểu là cố đẩy)
//   CONFIGURABLE-4: 2 nhân vật cùng vào 1 ô             (hiện: chặn)
//   CONFIGURABLE-5: 2 nhân vật đẩy cùng 1 hộp cùng lúc  (hiện: CHƯA XỬ LÝ ở
//                   đây — simulator vẫn áp dụng tuần tự A-rồi-B, xem
//                   resolveCollision() bên dưới và roadmap.md Cổng 4)
//   CONFIGURABLE-6: nhân vật đứng lên ô A/B được không? (hiện: được, không
//                   có luật nào chặn — xem canpush(), không kiểm tra ô đích)
// ========================================================

// Kiểm tra xem một nước đi/đẩy hộp có hợp lệ không
bool canpush(const gamestate& state, Pos agent, char dir) {
    if (dir == 'S') return true;
    if (dir != 'U' && dir != 'D' && dir != 'L' && dir != 'R') return false;

    auto [ddr, ddc] = getidx(dir);
    Pos nextPos = {agent.r + ddr, agent.c + ddc};

    if (state.grid.isWall(nextPos)) return false;

    // CONFIGURABLE-4: không đi đè lên nhân vật đối phương
    Pos other = (agent == state.a) ? state.b : state.a;
    if (state.grid.checkin(other) && nextPos == other) return false;

    // 1. Kiểm tra xem ô tiếp theo có hộp không
    if (state.hasBox(nextPos)) {
        // 2. Nếu có hộp: CONFIGURABLE-1 + CONFIGURABLE-2 — đẩy đúng 1 ô, đúng
        //    1 hộp theo Sokoban chuẩn. Muốn đẩy xa hơn 1 ô: đổi pushpos thành
        //    vòng lặp N ô. Muốn đẩy được dãy nhiều hộp: bỏ điều kiện chống
        //    đẩy 2 hộp liên tiếp bên dưới, thay bằng duyệt hết dãy hộp liền kề.
        Pos pushpos = {nextPos.r + ddr, nextPos.c + ddc};

        // hộp đụng tường / ra ngoài bàn
        if (state.grid.isWall(pushpos)) return false;

        // CONFIGURABLE-2: chống đẩy 2 hộp liên tiếp (chỉ đẩy được 1 hộp/lần)
        if (state.hasBox(pushpos)) return false;

        // CONFIGURABLE-4: không đẩy hộp đè lên nhân vật đối phương
        if (state.grid.checkin(other) && pushpos == other) return false;
    }

    return true;
}

// Thực thi nước đi và chấm điểm.
// Tự kiểm tra hợp lệ: nước đi sai bị coi như đứng yên thay vì đi xuyên tường.
// Trả về true nếu nước đi được chấp nhận.
bool applyMove(gamestate& state, bool isa, char dir) {
    Pos& agent = isa ? state.a : state.b;

    if (!canpush(state, agent, dir)) return false;
    if (dir == 'S') return true;

    auto [ddr, ddc] = getidx(dir);
    Pos nextPos = {agent.r + ddr, agent.c + ddc};

    // Nếu có đẩy hộp, di chuyển hộp và chấm điểm
    if (state.hasBox(nextPos)) {
        Pos newBoxPos = {nextPos.r + ddr, nextPos.c + ddc};

        // canpush đã bảo đảm newBoxPos hợp lệ, kiểm lại cho chắc
        if (!state.grid.checkin(newBoxPos)) return false;

        char destCell = state.grid.cell[newBoxPos.r][newBoxPos.c];
        if (destCell == 'A') {
            // FACT: hộp vào A -> điểm cho đội thí sinh, BẤT KỂ ai đẩy
            state.scoreA++;
            state.removeBox(nextPos);
        } else if (destCell == 'B') {
            // FACT: hộp vào B -> điểm cho đội đối thủ, BẤT KỂ ai đẩy
            state.scoreB++;
            state.removeBox(nextPos);
        } else {
            state.update(nextPos, newBoxPos);
        }
    }

    agent = nextPos;
    return true;
}

// ============================================================================
// resolveCollision — áp dụng nước đi của CẢ HAI bên trên CÙNG một trạng thái,
// thay vì tuần tự applyMove(A) rồi applyMove(B) (cách cũ khiến B "thấy" A đã
// đi rồi trong lần áp dụng, dù cả hai đã quyết định trên cùng 1 state trước
// đó — đây chính là nguồn gốc invalid-move rate ~0.1-0.2% đo được xuyên suốt
// mọi benchmark, đã ghi trong docs/eval_experiments.md).
//
// CONFIGURABLE-5 (đây là hàm giải quyết chính cho điểm này):
//   - 2 người cùng nhắm vào 1 ô trống -> mặc định: CẢ HAI ĐỨNG YÊN.
//   - 2 người cùng đẩy 1 hộp -> mặc định: HỘP ĐỨNG YÊN (không ai đẩy được).
//   - A đẩy hộp vào đúng ô B vừa rời đi -> CHO PHÉP (ô đó trống tại thời
//     điểm hộp tới, vì B đã di chuyển đi trong cùng lượt này).
//   - A đẩy hộp vào chính vị trí B hiện tại (B không rời khỏi đó) -> CHẶN,
//     giống luật "không đẩy hộp đè nhân vật đối phương" đã có, chỉ áp dụng
//     đúng cho trường hợp đồng thời.
//
// Đây là 1 giả định mặc định hợp lý (fail-safe: giữ nguyên trạng khi có
// tranh chấp), CHƯA PHẢI FACT vì luật va chạm đồng thời chưa được BTC công
// bố chi tiết. Khi có đề thật, đây là hàm đầu tiên cần đối chiếu.
// ============================================================================
struct MoveIntent {
    bool valid = false;     // nước đi có hợp lệ đơn phương không (không tính va chạm)
    Pos agentDest;          // vị trí agent sẽ tới nếu không có tranh chấp
    bool pushesBox = false;
    Pos boxSrc, boxDest;    // hộp bị đẩy (nếu có): từ đâu tới đâu
};

inline MoveIntent computeIntent(const gamestate& state, bool isa, char dir) {
    MoveIntent intent;
    Pos agent = isa ? state.a : state.b;

    if (!canpush(state, agent, dir)) {
        intent.valid = false;
        intent.agentDest = agent;
        return intent;
    }
    intent.valid = true;

    if (dir == 'S') {
        intent.agentDest = agent;
        return intent;
    }

    auto [ddr, ddc] = getidx(dir);
    Pos nextPos = {agent.r + ddr, agent.c + ddc};
    intent.agentDest = nextPos;

    if (state.hasBox(nextPos)) {
        intent.pushesBox = true;
        intent.boxSrc = nextPos;
        intent.boxDest = {nextPos.r + ddr, nextPos.c + ddc};
    }
    return intent;
}

// Áp dụng cả 2 nước đi moveA, moveB lên state đồng thời, có giải va chạm.
// Trả về {acceptedA, acceptedB} — true nếu nước đi đó thực sự được áp dụng
// (không bị va chạm chặn lại). Một nước đi hợp lệ đơn phương vẫn có thể bị
// từ chối ở đây nếu xung đột với nước đi kia.
//
// rngForTiebreak: PHÁT HIỆN THỰC NGHIỆM quan trọng (xem docs/eval_experiments.md
// Vòng 4-collision) — nếu quy tắc "cả 2 cùng vào 1 ô -> cả 2 đứng yên" luôn
// quyết định NHƯ NHAU (không có yếu tố ngẫu nhiên nào), 2 bot tất định
// (deterministic, không dùng RNG như botGreedy/botHeuristic) sẽ tính ra
// đúng cùng 1 nước xung đột đó ở MỌI lượt tiếp theo vì trạng thái không đổi
// -> deadlock vô hạn thật sự (đã đo được ~98% ván "stuck" khi chưa sửa).
// Truyền 1 RNG vào đây để tung đồng xu chọn 1 bên đi qua khi có tranh chấp
// thuần di chuyển (không áp dụng cho tranh chấp đẩy hộp — trường hợp đó vẫn
// chặn cả hai, ít khi lặp vì cần thẳng hàng cụ thể). Nếu không truyền RNG
// (nullptr), giữ hành vi cũ "cả hai đứng yên" — CONFIGURABLE, nhưng khuyến
// nghị luôn truyền RNG khi dùng cho simulator nhiều lượt.
inline pair<bool,bool> resolveCollision(gamestate& state, char moveA, char moveB,
                                          mt19937* rngForTiebreak = nullptr) {
    MoveIntent intentA = computeIntent(state, true,  moveA);
    MoveIntent intentB = computeIntent(state, false, moveB);

    bool acceptA = intentA.valid;
    bool acceptB = intentB.valid;

    // Ca 2 cung vao 1 o (khong tinh dung yen tai cho).
    if (acceptA && acceptB && moveA != 'S' && moveB != 'S'
        && intentA.agentDest == intentB.agentDest) {
        if (rngForTiebreak) {
            // Tung dong xu: 1 ben duoc di qua, ben kia dung yen. Pha vo
            // deadlock tat dinh giua 2 bot khong dung RNG rieng.
            uniform_int_distribution<int> coin(0, 1);
            if (coin(*rngForTiebreak) == 0) acceptB = false;
            else acceptA = false;
        } else {
            acceptA = acceptB = false;
        }
    }

    // Ca 2 cung day 1 hop -> tung dong xu tuong tu (cung ly do: neu luon
    // chan ca hai, 2 bot tat dinh se lap lai y het nuoc di nay moi luot).
    if (acceptA && acceptB && intentA.pushesBox && intentB.pushesBox
        && intentA.boxSrc == intentB.boxSrc) {
        if (rngForTiebreak) {
            uniform_int_distribution<int> coin(0, 1);
            if (coin(*rngForTiebreak) == 0) acceptB = false;
            else acceptA = false;
        } else {
            acceptA = acceptB = false;
        }
    }

    // A day hop vao vi tri B se toi (B cung dang di chuyen toi do) -> chan,
    // vi B se "chiem" o do trong cung luot.
    if (acceptA && intentA.pushesBox && moveB != 'S' && acceptB
        && intentA.boxDest == intentB.agentDest) {
        acceptA = false;
    }
    if (acceptB && intentB.pushesBox && moveA != 'S' && acceptA
        && intentB.boxDest == intentA.agentDest) {
        acceptB = false;
    }

    if (acceptA) applyMove(state, true,  moveA);
    if (acceptB) applyMove(state, false, moveB);

    return {acceptA, acceptB};
}

// ============================================================================
// isBoxDeadlocked — TASK DEADLOCK-01 (roadmap.md Cổng 5).
//
// Phát hiện hộp bị kẹt VĨNH VIỄN, không ai (bên nào) còn đẩy được nữa.
//
// Suy luận (không phải trực giác "kẹt góc" đơn thuần — đây là hệ quả toán
// học từ chính luật đẩy 1 ô/1 hộp hiện tại):
//   Để đẩy hộp theo hướng d, cần 2 điều kiện:
//     (1) ô "dest" (box + d)   không phải tường  -> hộp có chỗ để tới
//     (2) ô "stand" (box - d)  không phải tường  -> AGENT phải đứng được ở
//         đó trước khi đẩy (agent tiến vào hộp từ phía đối diện hướng đẩy)
//
//   Xét trục ngang (L/R): "stand" của push-RIGHT chính là "dest" của
//   push-LEFT (đều là ô (r, c-1)), và ngược lại. Do đó CHỈ CẦN 1 ô liền kề
//   theo trục ngang là tường (trái HOẶC phải) thì CẢ 2 hướng đẩy ngang đều
//   vĩnh viễn bất khả thi — không phải chỉ hướng có tường mới bị chặn.
//   Tương tự cho trục dọc (U/D).
//
//   => Hộp bị kẹt vĩnh viễn (không hướng nào đẩy được, mãi mãi, vì tường
//      không di chuyển) khi và chỉ khi:
//         (tường trái HOẶC tường phải)  VÀ  (tường trên HOẶC tường dưới)
//
// Điều kiện này ĐÚNG VỚI LUẬT ĐẨY HIỆN TẠI (CONFIGURABLE-1/2: đẩy đúng 1 ô/
// 1 hộp mỗi lần). Nếu đề thật đổi luật đẩy (ví dụ đẩy được nhiều ô), suy
// luận "stand trùng dest của hướng ngược lại" vẫn đúng nguyên tắc, chỉ cần
// đối chiếu lại CONFIGURABLE-1 khi porti.
// ============================================================================
inline bool isBoxDeadlocked(const Grid& g, Pos box) {
    bool wallLeft  = g.isWall({box.r, box.c - 1});
    bool wallRight = g.isWall({box.r, box.c + 1});
    bool wallUp    = g.isWall({box.r - 1, box.c});
    bool wallDown  = g.isWall({box.r + 1, box.c});
    return (wallLeft || wallRight) && (wallUp || wallDown);
}

// ============================================================================
// isFrozenSquare — mở rộng isBoxDeadlocked cho thế kẹt "khối vuông 2x2" giữa
// HỘP VỚI HỘP (isBoxDeadlocked chỉ xét tường quanh 1 hộp, không biết hộp khác
// cũng có thể đóng vai trò như tường tạm thời).
//
// Nếu 1 ô p (dự định đặt hộp vào đó) cùng với 2 ô kề trục + 1 ô chéo tạo
// thành 1 hình vuông 2x2 mà cả 4 ô đều là tường hoặc có hộp khác, thì không
// hộp nào trong khối đó còn đẩy được nữa: với hộp tại 1 góc bất kỳ của khối,
// mọi hướng đẩy đều cần "dest" hoặc "stand" trùng đúng 1 trong 2 ô kề trục
// còn lại của khối (đã bị chiếm) — xem chứng minh chi tiết ở bản gốc
// greedy_bot2/rules.cpp. Đây là luật deadlock chuẩn, không phụ thuộc đích ở
// đâu nên không bao giờ chặn nhầm 1 nước đi còn khả thi (chỉ có thể bỏ sót
// thế kẹt phức tạp hơn 2x2, không bao giờ false-positive).
inline bool isFrozenSquare(const Grid& g, Pos p, const vector<Pos>& otherBoxes) {
    if (g.cell[p.r][p.c] == 'A' || g.cell[p.r][p.c] == 'B') return false;

    auto occupied = [&](Pos q) {
        if (g.isWall(q)) return true;
        for (const Pos& b : otherBoxes) if (b == q) return true;
        return false;
    };

    const int quadR[] = {-1, -1, 1, 1};
    const int quadC[] = {-1, 1, -1, 1};
    for (int k = 0; k < 4; ++k) {
        Pos horiz = {p.r, p.c + quadC[k]};
        Pos vert  = {p.r + quadR[k], p.c};
        Pos diag  = {p.r + quadR[k], p.c + quadC[k]};
        if (occupied(horiz) && occupied(vert) && occupied(diag)) return true;
    }
    return false;
}
