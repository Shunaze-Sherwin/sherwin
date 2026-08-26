// ============================================================================
// submit.cpp — BAN GOP 1 FILE DUY NHAT DE NOP THI.
//
// Gop tu (theo dung thu tu phu thuoc, KHONG doi logic mot dong nao):
//   src/core/grid.cpp, src/game/state.cpp, src/search/bfs.cpp,
//   src/core/timer.cpp, src/game/rules.cpp, src/evaluation/evaluate.cpp,
//   src/bots/bot_random.cpp, src/bots/bot_greedy.cpp,
//   src/bots/bot_heuristic.cpp, src/game/io_protocol.cpp, main.cpp
//
// Build:  g++ -std=c++17 -O2 submit.cpp -o submit
// Chay:   ./submit < input.txt          (mac dinh: doc 1 lan, in 1 nuoc, thoat)
//         ./submit --interactive        (doc lap lai nhieu luot qua 1 process song)
//
// LUU Y: botHeuristic() doc configs/eval_weights.json neu file ton tai cung
// thu muc luc chay (khong bat buoc phai co -- thieu file thi tu dong dung
// trong so mac dinh trong loadWeights(), khong crash). DEFAULT_BOT hien tai
// la GREEDY nen file json nay khong anh huong gi den bot dang nop.
//
// File nay CHI ghep noi dung, khong sua logic -- neu can sua luat/IO/bot,
// sua trong ban nhieu-file (thu muc THT goc) roi chay lai script nay, DUNG
// sua truc tiep trong file gop nay (de tranh 2 ban lech nhau).
// ============================================================================
#include <bits/stdc++.h>
using namespace std;


// ---------------------------------------------------------------------------
// Tu: src/core/grid.cpp
// ---------------------------------------------------------------------------
// Tọa độ trên bàn cờ
struct Pos {
    int r, c;
    bool operator==(const Pos& other) const {
        return r == other.r && c == other.c;
    }
    bool operator!=(const Pos& other) const {
        return !(*this == other);
    }
};
// Cấu trúc bàn cờ 16x16
struct Grid {
    char cell[16][16];

    // Kiểm tra tọa độ có nằm trong bàn cờ không
    bool checkin(Pos p) const {
        return p.r >= 0 && p.r < 16 && p.c >= 0 && p.c < 16;
    }

    // Kiểm tra ô có phải là tường không (ngoài biên mặc định là tường)
    bool isWall(Pos p) const {
        if (!checkin(p)) return true;
        return cell[p.r][p.c] == '#';
    }

    // Kiểm tra ô có thể đi vào được không (mọi ô không phải tường)
    bool isFree(Pos p) const {
        return checkin(p) && cell[p.r][p.c] != '#';
    }
};

// Mặt nạ ô bị chiếm (hộp, nhân vật đối phương...) — dùng cho BFS đường đi của agent
typedef array<array<bool, 16>, 16> Occupancy;

inline Occupancy emptyOccupancy() {
    Occupancy occ{};
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c) occ[r][c] = false;
    return occ;
}

// Hàm đọc bàn cờ từ dữ liệu đầu vào (giả định Input là 16 dòng string)
Grid inp(const vector<string>& lines) {
    Grid g;
    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            g.cell[r][c] = lines[r][c];
        }
    }
    return g;
}

// Hàm in bàn cờ ra màn hình để debug
void print(const Grid& g) {
    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            cout << g.cell[r][c];
        }
        cout << '\n';
    }
}

// Hàm sinh bản đồ ngẫu nhiên phục vụ Stress Test (Task 2)

// ---------------------------------------------------------------------------
// Tu: src/game/state.cpp
// ---------------------------------------------------------------------------
struct Move {
    char dir; // 'U' (Lên), 'D' (Xuống), 'L' (Trái), 'R' (Phải), 'S' (Đứng yên)
};

// Cấu trúc Trạng thái của 1 ván đấu tại một thời điểm
struct gamestate {
    Grid grid;             // Bản đồ TĨNH: chỉ chứa '.', '#', 'A', 'B' (không chứa a/b/X)
    Pos a = {-1, -1};      // Tọa độ nhân vật phe ta
    Pos b = {-1, -1};      // Tọa độ nhân vật đối thủ
    vector<Pos> boxes;     // Danh sách tọa độ các hộp CÒN TRÊN BÀN
    int scoreA = 0;        // Điểm phe ta
    int scoreB = 0;        // Điểm phe địch

    bool hasBox(Pos p) const {
        for (const Pos& box : boxes) if (box == p) return true;
        return false;
    }

    // Di chuyển hộp từ oldPos sang newPos
    void update(Pos oldPos, Pos newPos) {
        for (auto& box : boxes) {
            if (box == oldPos) {
                box = newPos;
                return;
            }
        }
    }

    // Hộp đã vào đích -> XOÁ HẲN khỏi danh sách.
    // (Trước đây gán {-1,-1} và giữ lại, gây truy cập dist[-1][-1] ngoài mảng.)
    void removeBox(Pos p) {
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i] == p) {
                boxes.erase(boxes.begin() + i);
                return;
            }
        }
    }
};

// Tách các thực thể động (a, b, X) khỏi bản đồ tĩnh và nạp vào gamestate.
// Sau khi gọi, state.grid chỉ còn '.', '#', 'A', 'B'.
gamestate makeStateFromGrid(const Grid& raw) {
    gamestate st;
    st.grid = raw;
    st.boxes.clear();

    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            char ch = raw.cell[r][c];
            if (ch == 'a') { st.a = {r, c}; st.grid.cell[r][c] = '.'; }
            else if (ch == 'b') { st.b = {r, c}; st.grid.cell[r][c] = '.'; }
            else if (ch == 'X') { st.boxes.push_back({r, c}); st.grid.cell[r][c] = '.'; }
        }
    }
    return st;
}

// Dựng mặt nạ ô bị chiếm: mọi hộp + nhân vật đối phương.
Occupancy buildOccupancy(const gamestate& state, bool isa) {
    Occupancy occ = emptyOccupancy();
    for (const Pos& box : state.boxes) {
        if (state.grid.checkin(box)) occ[box.r][box.c] = true;
    }
    Pos other = isa ? state.b : state.a;
    if (state.grid.checkin(other)) occ[other.r][other.c] = true;
    return occ;
}

// Chữ ký trạng thái (vị trí agent + tập hộp đã sort) — dùng để phát hiện
// lặp trạng thái (dấu hiệu bot bị kẹt, "đổi ý giữa chừng" nhiều lần liên
// tiếp). Không cần hash phức tạp (kiểu Zobrist) ở quy mô bàn 16x16/vài
// nghìn lượt kiểm tra — so sánh string trực tiếp đủ nhanh (TASK ROBUST-03).
inline string stateSignature(const gamestate& state) {
    vector<Pos> sorted = state.boxes;
    sort(sorted.begin(), sorted.end(), [](const Pos& x, const Pos& y) {
        return x.r != y.r ? x.r < y.r : x.c < y.c;
    });
    string sig;
    sig.reserve(16 + sorted.size() * 8);
    sig += to_string(state.a.r) + "," + to_string(state.a.c) + "|";
    sig += to_string(state.b.r) + "," + to_string(state.b.c) + "|";
    for (const Pos& p : sorted) sig += to_string(p.r) + "," + to_string(p.c) + ";";
    return sig;
}

void printstate(const gamestate& state) {
    Grid displayGrid = state.grid;
    for (Pos box : state.boxes) {
        if (displayGrid.checkin(box)) displayGrid.cell[box.r][box.c] = 'X';
    }
    if (displayGrid.checkin(state.a)) displayGrid.cell[state.a.r][state.a.c] = 'a';
    if (displayGrid.checkin(state.b)) displayGrid.cell[state.b.r][state.b.c] = 'b';

    print(displayGrid);
    cout << "Diem A: " << state.scoreA << " | Diem B: " << state.scoreB
         << " | Hop con lai: " << state.boxes.size() << "\n";
    cout << "----------------------\n";
}

// ---------------------------------------------------------------------------
// Tu: src/search/bfs.cpp
// ---------------------------------------------------------------------------
const int INF = 1e9;

int dr[] = {-1, 1, 0, 0};
int dc[] = {0, 0, -1, 1};
const char DIRCH[] = {'U', 'D', 'L', 'R'};

// (Occupancy / emptyOccupancy được định nghĩa trong core/grid.cpp)

// BFS cơ bản: chỉ né tường. Dùng để ước lượng khoảng cách của HỘP (hộp đẩy được qua ô có agent).
vector<vector<int>> bfsFrom(const Grid& g, Pos start) {
    vector<vector<int>> dist(16, vector<int>(16, INF));

    if (g.isWall(start)) return dist;

    queue<Pos> q;
    q.push(start);
    dist[start.r][start.c] = 0;

    while (!q.empty()) {
        Pos u = q.front();
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Pos v = {u.r + dr[i], u.c + dc[i]};
            if (g.isFree(v) && dist[v.r][v.c] == INF) {
                dist[v.r][v.c] = dist[u.r][u.c] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

// BFS cho AGENT: né tường VÀ né các ô bị chiếm (hộp, nhân vật đối phương).
// Ô xuất phát luôn hợp lệ kể cả khi đang bị đánh dấu chiếm.
vector<vector<int>> bfsAvoid(const Grid& g, Pos start, const Occupancy& occ) {
    vector<vector<int>> dist(16, vector<int>(16, INF));

    if (!g.isFree(start)) return dist;

    queue<Pos> q;
    q.push(start);
    dist[start.r][start.c] = 0;

    while (!q.empty()) {
        Pos u = q.front();
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Pos v = {u.r + dr[i], u.c + dc[i]};
            if (!g.isFree(v)) continue;
            if (occ[v.r][v.c]) continue;
            if (dist[v.r][v.c] != INF) continue;
            dist[v.r][v.c] = dist[u.r][u.c] + 1;
            q.push(v);
        }
    }
    return dist;
}

// BFS đa nguồn: khoảng cách từ MỌI ô tới ô đích gần nhất mang ký tự goalChar ('A' hoặc 'B').
// Dùng để đo "hộp này còn cách đích của mình bao xa".
vector<vector<int>> bfsToGoals(const Grid& g, char goalChar) {
    vector<vector<int>> dist(16, vector<int>(16, INF));
    queue<Pos> q;

    for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            if (g.cell[r][c] == goalChar) {
                dist[r][c] = 0;
                q.push({r, c});
            }
        }
    }

    while (!q.empty()) {
        Pos u = q.front();
        q.pop();
        for (int i = 0; i < 4; ++i) {
            Pos v = {u.r + dr[i], u.c + dc[i]};
            if (g.isFree(v) && dist[v.r][v.c] == INF) {
                dist[v.r][v.c] = dist[u.r][u.c] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

// Đếm số ô "độc quyền" mà Agent A hoặc Agent B kiểm soát
pair<int, int> cnt(const Grid& g, Pos a, Pos b) {
    vector<vector<int>> distA = bfsFrom(g, a);
    vector<vector<int>> distB = bfsFrom(g, b);

    int scoreA = 0, scoreB = 0;
        for (int r = 0; r < 16; ++r) {
        for (int c = 0; c < 16; ++c) {
            if (g.isWall({r, c})) continue;

            if (distA[r][c] < distB[r][c]) {
                scoreA++;
            } else if (distB[r][c] < distA[r][c]) {
                scoreB++;
            }
        }
    }
    return {scoreA, scoreB};
}

// Bước đi đầu tiên từ 'from' tới 'to', né các ô bị chiếm. Trả 'S' nếu không tới được.
// (Đổi tên từ find() để không đụng std::find khi có using namespace std.)
char firstStepTowards(const Grid& g, Pos from, Pos to, const Occupancy& occ) {
    if (from == to) return 'S';
    if (!g.isFree(to) || occ[to.r][to.c]) return 'S';

    vector<vector<int>> dist = bfsAvoid(g, to, occ);
    if (dist[from.r][from.c] == INF) return 'S';

    int current_dist = dist[from.r][from.c];

    for (int i = 0; i < 4; ++i) {
        Pos next_pos = {from.r + dr[i], from.c + dc[i]};
        if (!g.isFree(next_pos)) continue;
        if (occ[next_pos.r][next_pos.c]) continue;
        if (dist[next_pos.r][next_pos.c] == current_dist - 1) {
            return DIRCH[i];
        }
    }

    return 'S';
}

// Bản tiện lợi: không có ô bị chiếm nào
char firstStepTowards(const Grid& g, Pos from, Pos to) {
    return firstStepTowards(g, from, to, emptyOccupancy());
}

// ---------------------------------------------------------------------------
// Tu: src/core/timer.cpp
// ---------------------------------------------------------------------------
#include <chrono>
// Cấu trúc Đồng hồ đếm ngược
struct Timer {
    chrono::time_point<chrono::high_resolution_clock> startTime;
    double timelimit; 
    Timer(double limit) {
        startTime = chrono::high_resolution_clock::now();
        timelimit = limit;
    }
    double elapsed() const {
        auto now = chrono::high_resolution_clock::now();
        return chrono::duration<double, milli>(now - startTime).count();
    }
    bool istimeup(double margin = 20.0) const {
        return elapsed() >= (timelimit - margin);
    }
};
// ---------------------------------------------------------------------------
// Tu: src/game/rules.cpp
// ---------------------------------------------------------------------------
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

    // CONFIGURABLE-6: nhân vật đứng lên ô A/B được không? (hiện: được — dòng
    // dưới đây bị comment. Nếu đề thật cấm việc này, bỏ comment dòng if bên
    // dưới. LƯU Ý phát hiện qua MOCK-01: trước bản sửa này, đây là điểm
    // CONFIGURABLE duy nhất trong file được liệt kê ở đầu file (dòng ~30)
    // nhưng KHÔNG có marker inline tại đúng chỗ cần sửa — mất thời gian dò
    // tìm thật sự khi thử đổi luật này. Marker đã được thêm ngay tại đây.)
    char destCell = state.grid.cell[nextPos.r][nextPos.c];
    // if (destCell == 'A' || destCell == 'B') return false;
    (void)destCell;   // tranh warning "unused variable" khi dong if o tren dang comment

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

// ---------------------------------------------------------------------------
// Tu: src/evaluation/evaluate.cpp
// ---------------------------------------------------------------------------
// ==========================================
// TRỌNG SỐ EVALUATION
//
// 2 cơ chế song song, không loại trừ nhau:
//   1) #define ghi đè lúc build (-DW_AGENT_TO_BOX=20) -> dùng cho quét thô
//      hàng loạt tổ hợp tự động hoá (đã dùng ở Vòng 1, xem eval_experiments.md).
//   2) configs/eval_weights.json đọc lúc chạy -> dùng để chỉnh tay nhanh
//      giữa 120 phút thách đấu KHÔNG CẦN build lại code.
// #define là giá trị fallback nếu file JSON thiếu/lỗi -> không bao giờ
// crash chỉ vì thiếu file cấu hình.
// ==========================================
#ifndef W_SCORE_DIFF
#define W_SCORE_DIFF 1000     // Feature 1: chênh lệch điểm
#endif
#ifndef W_BOX_TO_GOAL
#define W_BOX_TO_GOAL 60      // Feature 2: hộp càng gần đích của mình càng tốt
#endif
#ifndef W_AGENT_TO_BOX
#define W_AGENT_TO_BOX 2      // Feature 3: nhân vật càng gần ô đứng đẩy càng tốt
#endif
#ifndef W_DEADLOCK
#define W_DEADLOCK 300        // Feature 5: phạt hộp bị kẹt vĩnh viễn (DEADLOCK-01)
#endif
// Giá trị 60/2 lấy từ vòng quét thô 20 tổ hợp (xem docs/eval_experiments.md
// Vòng 1), đã xác nhận ổn định ở Vòng 2. Đây là bộ trọng số CHỐT TẠM, không
// phải cuối cùng — xem roadmap.md Cổng 5 (BOT3-COMMIT-01) trước khi tune
// thêm F2/F3, vì đã chạm trần ~46% do vấn đề cấu trúc, không phải trọng số.

struct EvalWeights {
    int scoreDiff  = W_SCORE_DIFF;
    int boxToGoal  = W_BOX_TO_GOAL;
    int agentToBox = W_AGENT_TO_BOX;
    int deadlock   = W_DEADLOCK;
};

// Parse tối giản 1 file JSON phẳng dạng {"KEY": số, ...} — không cần thư viện
// JSON đầy đủ vì chỉ có 3 key số nguyên. Không tìm thấy key nào -> giữ giá
// trị #define mặc định cho key đó. File thiếu/lỗi hoàn toàn -> trả về mặc
// định #define cho cả 3, KHÔNG throw, KHÔNG crash.
inline EvalWeights loadWeights(const string& path = "configs/eval_weights.json") {
    EvalWeights w;   // đã khởi tạo bằng #define mặc định
    ifstream f(path);
    if (!f.is_open()) return w;

    try {
        stringstream buf;
        buf << f.rdbuf();
        string text = buf.str();

        auto extract = [&](const string& key, int& target) {
            size_t pos = text.find("\"" + key + "\"");
            if (pos == string::npos) return;
            size_t colon = text.find(':', pos);
            if (colon == string::npos) return;
            size_t numStart = colon + 1;
            while (numStart < text.size() && (text[numStart] == ' ' || text[numStart] == '\t')) numStart++;
            size_t numEnd = numStart;
            bool neg = false;
            if (numEnd < text.size() && text[numEnd] == '-') { neg = true; numEnd++; }
            size_t digitsStart = numEnd;
            while (numEnd < text.size() && isdigit((unsigned char)text[numEnd])) numEnd++;
            if (numEnd == digitsStart) return;   // không có chữ số nào -> giữ mặc định
            int val = stoi(text.substr(digitsStart, numEnd - digitsStart));
            target = neg ? -val : val;
        };

        extract("W_SCORE_DIFF", w.scoreDiff);
        extract("W_BOX_TO_GOAL", w.boxToGoal);
        extract("W_AGENT_TO_BOX", w.agentToBox);
        extract("W_DEADLOCK", w.deadlock);
    } catch (...) {
        return EvalWeights{};   // bất kỳ lỗi nào -> quay về #define mặc định an toàn
    }
    return w;
}

// Cache trọng số: đọc file 1 lần duy nhất, không đọc lại mỗi lần gọi
// evaluatestate() (hàm này được gọi rất nhiều lần trong 1 lượt của Bot 3 —
// tới 5 lần lookahead — đọc file mỗi lần sẽ chậm đáng kể).
inline const EvalWeights& currentWeights() {
    static EvalWeights cached = loadWeights();
    return cached;
}

const int DIST_CAP = 40;          // giá trị thay cho INF khi ô không tới được

// Hàm chấm điểm (Quality) cho trạng thái, nhìn từ phía isa.
int evaluatestate(const gamestate& state, bool isa) {
    const EvalWeights& W = currentWeights();
    int quality = 0;

    // ==========================================
    // FEATURE 1: Chênh lệch điểm số
    // ==========================================
    int myScore  = isa ? state.scoreA : state.scoreB;
    int oppScore = isa ? state.scoreB : state.scoreA;
    quality += (myScore - oppScore) * W.scoreDiff;

    if (state.boxes.empty()) return quality;

    // ==========================================
    // FEATURE 2: Tổng khoảng cách các hộp -> ô đích CỦA MÌNH
    // (phe ta nhắm 'A', phe địch nhắm 'B' — trước đây thiếu hẳn feature này,
    //  nên bot không có động lực đẩy hộp về đích.)
    // ==========================================
    char myGoal = isa ? 'A' : 'B';
    vector<vector<int>> distToGoal = bfsToGoals(state.grid, myGoal);

    for (const Pos& box : state.boxes) {
        int d = distToGoal[box.r][box.c];
        quality -= min(d, DIST_CAP) * W.boxToGoal;
    }

    // ==========================================
    // FEATURE 5: Phạt hộp bị kẹt vĩnh viễn (TASK DEADLOCK-01)
    //
    // isBoxDeadlocked() (rules.cpp) xác nhận toán học: hộp không còn hướng
    // đẩy nào khả thi, mãi mãi (tường không di chuyển). Phạt nặng MỖI hộp
    // đang ở trạng thái này, để:
    //   (a) Bot 3 (Heuristic, dùng evaluatestate trực tiếp) tránh chủ động
    //       đẩy 1 hộp vào thế kẹt — nếu 1 nước đi khiến hộp rơi vào deadlock,
    //       quality của trạng thái SAU nước đi đó bị phạt nặng ngay lập tức
    //       (evaluatestate luôn chấm nextState, nên phát hiện đúng lúc).
    //   (b) Gián tiếp giảm động lực F2 cộng dồn khoảng cách vô nghĩa của hộp
    //       không bao giờ tới đích được nữa.
    // Không ảnh hưởng Bot 1 (Greedy) vì bot này không dùng evaluatestate —
    // xem thêm phần chặn tương ứng trong bot_greedy.cpp.
    // ==========================================
    for (const Pos& box : state.boxes) {
        if (isBoxDeadlocked(state.grid, box)) {
            quality -= W.deadlock;
        }
    }

    // ==========================================
    // FEATURE 3: Khoảng cách nhân vật -> Ô ĐỨNG ĐẨY HỮU ÍCH gần nhất
    // (không phải khoảng cách tới hộp: đứng sát hộp nhưng SAI PHÍA thì vô dụng,
    //  và bản cũ dùng cứng boxes[0] chứ không phải hộp gần nhất)
    // ==========================================
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    if (!state.grid.checkin(agent)) return quality;

    Occupancy occ = buildOccupancy(state, isa);
    vector<vector<int>> distFromAgent = bfsAvoid(state.grid, agent, occ);

    int nearest = DIST_CAP;
    for (const Pos& box : state.boxes) {
        for (int i = 0; i < 4; ++i) {
            Pos dest  = {box.r + dr[i], box.c + dc[i]};   // hộp sẽ tới đây
            Pos stand = {box.r - dr[i], box.c - dc[i]};   // phải đứng đây mới đẩy được

            if (!state.grid.isFree(dest)  || state.hasBox(dest))  continue;
            if (!state.grid.isFree(stand) || state.hasBox(stand)) continue;
            if (state.grid.checkin(other) && (dest == other || stand == other)) continue;

            // Cú đẩy này phải đưa hộp lại GẦN đích của mình
            int dBox  = min(distToGoal[box.r][box.c],   DIST_CAP);
            int dDest = min(distToGoal[dest.r][dest.c], DIST_CAP);
            if (dDest >= dBox) continue;

            int cost = (agent == stand) ? 0 : distFromAgent[stand.r][stand.c];
            nearest = min(nearest, min(cost, DIST_CAP));
        }
    }
    quality -= nearest * W.agentToBox;

    return quality;
}

// ---------------------------------------------------------------------------
// Tu: src/bots/bot_random.cpp
// ---------------------------------------------------------------------------
// Bot Random: Liệt kê các nước đi hợp lệ và chọn ngẫu nhiên
char botRandom(const gamestate& state, bool isa, mt19937& rng) {
    Pos agent = isa ? state.a : state.b;
    string answer = "";
    char dirs[] = {'U', 'D', 'L', 'R', 'S'};
    
    for (char d : dirs) {
        if (canpush(state, agent, d)) {
            answer += d;
        }
    }
    
    if (answer.empty()) return 'S';
    
    // Rút thăm ngẫu nhiên 1 nước đi trong danh sách hợp lệ
    uniform_int_distribution<int> dist(0, answer.size() - 1);
    return answer[dist(rng)];
}
// ---------------------------------------------------------------------------
// Tu: src/bots/bot_greedy.cpp
// ---------------------------------------------------------------------------
const int GREEDY_DIST_CAP = 40;

// TASK STEP-03 (roadmap.md) — port tu src/bots/bot_gready_upgrade.cpp (thu
// nghiem, chua tich hop chinh thuc truoc day). Mo rong isBoxDeadlocked
// (chi xet 1 hop voi tuong xung quanh no) cho the ket "khoi vuong 2x2"
// giua HOP VOI HOP (isBoxDeadlocked khong biet hop khac cung co the dong
// vai tro nhu tuong tam thoi).
//
// Neu 1 o p (du dinh dat hop vao do) cung voi 2 o ke truc + 1 o cheo tao
// thanh 1 hinh vuong 2x2 ma ca 4 o deu la tuong hoac co hop khac, thi khong
// hop nao trong khoi do con day duoc nua. Day la luat deadlock chuan, chi
// co the bo sot the ket phuc tap hon 2x2, khong bao gio chan nham 1 nuoc
// di con kha thi.
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

// BOT 1 — Greedy nearest box, NHƯNG có nhắm ô đích.
// Chọn cặp (hộp, hướng đẩy) sao cho cú đẩy làm hộp TIẾN GẦN ô đích của mình,
// rồi đi tới ô đứng cần thiết để thực hiện cú đẩy đó.
// (Bản cũ chỉ đi tới hộp gần nhất rồi đẩy theo hướng BFS tuỳ ý, nên hộp bị
//  đẩy đi lung tung và gần như không bao giờ ghi được điểm.)
//
// Timer: kiểm tra istimeup() mỗi 8 lần lặp (không phải mỗi lần lặp, để
// tránh overhead gọi chrono::now() quá thường xuyên trên vòng lặp nhỏ
// numBoxes*4). Nếu hết giờ giữa chừng, dùng ngay kết quả tốt nhất đã có.
//
// TASK STEP-06 (roadmap.md, TIEBREAK-01): tham số `rng` là con trỏ, mặc
// định nullptr -- CÙNG idiom với resolveCollision() trong rules.cpp (xem
// test11/test12/test13 trong tests/test_push.cpp). rng == nullptr giữ
// nguyên hành vi tất định gốc (mọi caller cũ không truyền rng không đổi
// hành vi). rng != nullptr: khi 2 ứng viên (hộp, hướng) có cost VÀ gain
// bằng nhau tuyệt đối, tung đồng xu quyết định giữ ứng viên cũ hay đổi
// sang ứng viên mới, thay vì luôn giữ ứng viên tìm thấy trước (thứ tự
// duyệt hộp/hướng). Lý do cần: log Vòng 4 (docs/eval_experiments.md) đã
// chứng minh 2 bot tất định gặp đúng 1 lựa chọn có thể tạo dao động vô hạn
// nếu không có yếu tố ngẫu nhiên -- resolveCollision() đã vá ở tầng
// simulator, đây là vá tương tự ở tầng quyết định của chính bot.
char botGreedy(const gamestate& state, bool isa, const Timer& timer, mt19937* rng = nullptr) {
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
    for (size_t bi = 0; bi < state.boxes.size(); ++bi) {
        if (timedOut) break;
        const Pos& box = state.boxes[bi];

        // TASK DEADLOCK-01: hộp đã kẹt vĩnh viễn (isBoxDeadlocked) không còn
        // hướng đẩy khả thi nào (đã chứng minh trong rules.cpp) -- bỏ qua
        // ngay, không phí thời gian duyệt 4 hướng cho hộp này (mọi hướng đều
        // sẽ fail điều kiện dest/stand free bên dưới, nhưng skip sớm cho rõ
        // ý và tiết kiệm 1 vòng lặp con).
        if (isBoxDeadlocked(state.grid, box)) continue;

        // STEP-03: can danh sach "hop khac" (khong tinh chinh hop dang xet,
        // loai theo INDEX chu khong theo gia tri toa do -- an toan ngay ca
        // trong truong hop hiem gap 2 hop trung vi tri) de isFrozenSquare
        // biet hop nao co the dong vai tro tuong tam thoi.
        vector<Pos> otherBoxes;
        for (size_t bj = 0; bj < state.boxes.size(); ++bj)
            if (bj != bi) otherBoxes.push_back(state.boxes[bj]);

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
            // STEP-03: cung 1 ngoai le destIsGoal ap dung cho isFrozenSquare
            // -- neu dest la o dich, hop ghi diem va bien mat ngay, khong
            // bao gio thuc su "ket" du 2x2 quanh no co ve bi khoa.
            if (!destIsGoal && isFrozenSquare(state.grid, dest, otherBoxes)) continue;

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
            } else if (rng && cost == bestCost && gain == bestGain) {
                // STEP-06 (TIEBREAK-01): tie tuyet doi -- tung dong xu 50/50
                // thay vi luon giu ung vien tim thay truoc. Khong doi
                // bestCost/bestGain (van bang nhau), chi co the doi
                // bestStand/bestDir.
                uniform_int_distribution<int> coin(0, 1);
                if (coin(*rng) == 1) {
                    bestStand = stand;
                    bestDir = DIRCH[i];
                }
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

// ---------------------------------------------------------------------------
// Tu: src/bots/bot_heuristic.cpp
// ---------------------------------------------------------------------------
// BOT 3 — Heuristic: nhìn trước 1 bước, chấm điểm trạng thái kết quả.
//
// Timer thật sự cắt được: fallback (nước hợp lệ đầu tiên tìm thấy, tương
// đương botRandom rút gọn) được tính TRƯỚC TIÊN và luôn có sẵn để trả về.
// Sau đó vòng lặp tìm nước tốt nhất tự kiểm tra timer.istimeup() giữa các
// lần đánh giá — nếu hết giờ, dừng ngay và trả kết quả tốt nhất đã có tới
// thời điểm đó (không phải chờ đánh giá hết cả 5 hướng rồi mới hỏi hết giờ
// chưa như bản cũ).
char botHeuristic(const gamestate& state, bool isa, const Timer& timer) {
    Pos agent = isa ? state.a : state.b;
    if (!state.grid.checkin(agent)) return 'S';

    char dirs[] = {'U', 'D', 'L', 'R', 'S'};

    // Fallback an toàn: nước hợp lệ đầu tiên tìm thấy, tính trước mọi thứ khác.
    char fallback = 'S';
    for (char d : dirs) {
        if (canpush(state, agent, d)) { fallback = d; break; }
    }

    char bestMove = fallback;
    int bestQuality = INT_MIN;

    for (char d : dirs) {
        if (timer.istimeup()) break;   // hết giờ giữa chừng -> dừng ngay, trả bestMove hiện có

        if (!canpush(state, agent, d)) continue;

        gamestate nextState = state;
        if (!applyMove(nextState, isa, d)) continue;

        int quality = evaluatestate(nextState, isa);
        if (quality > bestQuality) {
            bestQuality = quality;
            bestMove = d;
        }
    }

    return bestMove;
}

// Bản tương thích ngược: không truyền Timer -> coi như không giới hạn thời gian
// (dùng time-limit rất lớn). Giữ lại để không phá vỡ lời gọi cũ nếu còn sót.
char botHeuristic(const gamestate& state, bool isa) {
    Timer unlimited(1e9);
    return botHeuristic(state, isa, unlimited);
}

// ============================================================================
// TASK BOT3-COMMIT-01 (roadmap.md Cổng 5).
//
// Vấn đề đã chẩn đoán từ docs/eval_experiments.md Vòng 2 và xác nhận lại rõ
// hơn ở Vòng 6 (sau DEADLOCK-01): botHeuristic() ở trên chấm điểm lại TOÀN
// BỘ trạng thái sau đúng 1 bước mỗi lượt, không nhớ nó đang nhắm hộp/hướng
// nào — nên khi nước đi hấp dẫn nhất vừa bị loại (ví dụ do DEADLOCK-01 chặn
// đúng 1 cú đẩy sẽ tạo deadlock), nó không chủ động tìm SANG hộp khác mà có
// xu hướng "đóng băng" ở nước trung lập gần nhất (thường là 'S') vì F3 phạt
// việc rời xa ô-đứng-đẩy hữu ích cuối cùng nó biết, trong khi 1-step
// lookahead không đủ tầm nhìn để phát hiện lợi ích của việc di chuyển sang
// khu vực khác.
//
// botHeuristicCommitted() sửa đúng gốc rễ: NHỚ (hộp, hướng đẩy) đã chọn qua
// BotMemory (truyền vào/ra bằng tham chiếu), CHỈ đổi mục tiêu khi mục tiêu
// cũ không còn khả thi (hộp biến mất/bị đẩy đi nơi khác/dest giờ deadlock)
// — không đánh giá lại toàn bộ bàn cờ mỗi lượt.
//
// Khác biệt có chủ đích với botGreedy (để 2 bot không trở thành bản sao của
// nhau): khi PHẢI chọn mục tiêu mới, ưu tiên GAIN LỚN NHẤT trước (đưa hộp
// tiến gần đích nhiều nhất), hoà thì mới xét cost nhỏ nhất — botGreedy làm
// ngược lại (ưu tiên cost nhỏ nhất trước). Đây là 1 lựa chọn chiến thuật
// khác, cần A/B để biết có tốt hơn không, không phải giả định.
//
// Ràng buộc kiến trúc quan trọng: BotMemory chỉ sống được trong RAM giữa
// các lượt nếu chương trình gọi hàm này KHÔNG bị exec lại mỗi lượt (đúng với
// benchmarks/bench_bot_winrate.cpp, vòng lặp giữ state sống suốt 1 ván). Nếu
// dùng trong main.cpp (entry point thi thật, có thể bị exec lại mỗi lượt
// theo ASSUMPTION ở đó), phải đọc/ghi BotMemory từ 1 file tạm — CHƯA LÀM ở
// đây, main.cpp hiện vẫn dùng botHeuristic gốc (không commit) hoặc Bot 1
// làm DEFAULT_BOT, xem TODO trong main.cpp nếu muốn bật bot này khi thi.
// ============================================================================
struct BotMemory {
    bool hasTarget = false;
    Pos targetBox = {-1, -1};
    char targetDir = 'S';
};

char botHeuristicCommitted(const gamestate& state, bool isa, const Timer& timer, BotMemory& mem) {
    Pos agent = isa ? state.a : state.b;
    Pos other = isa ? state.b : state.a;
    if (!state.grid.checkin(agent)) return 'S';
    if (state.boxes.empty()) { mem.hasTarget = false; return 'S'; }

    char myGoal = isa ? 'A' : 'B';
    Occupancy occ = buildOccupancy(state, isa);
    vector<vector<int>> distAgent = bfsAvoid(state.grid, agent, occ);
    vector<vector<int>> distGoal  = bfsToGoals(state.grid, myGoal);

    // --- Kiểm tra commitment hiện tại còn khả thi không ---
    auto commitmentStillValid = [&]() -> bool {
        if (!mem.hasTarget) return false;
        if (!state.hasBox(mem.targetBox)) return false;   // hộp không còn ở đó (bị đẩy đi/ghi điểm)
        Pos box = mem.targetBox;
        auto [ddr, ddc] = getidx(mem.targetDir);
        Pos dest  = {box.r + ddr, box.c + ddc};
        Pos stand = {box.r - ddr, box.c - ddc};
        if (!state.grid.isFree(dest) || state.hasBox(dest)) return false;
        if (state.grid.checkin(other) && dest == other) return false;
        if (!state.grid.isFree(stand) || state.hasBox(stand)) return false;
        if (state.grid.checkin(other) && stand == other) return false;
        char destCell = state.grid.cell[dest.r][dest.c];
        bool destIsGoal = (destCell == 'A' || destCell == 'B');
        if (!destIsGoal && isBoxDeadlocked(state.grid, dest)) return false;
        return true;
    };

    if (!commitmentStillValid()) {
        // --- Chọn mục tiêu mới: ưu tiên gain lớn nhất trước (khác botGreedy) ---
        bool found = false;
        int bestGain = -1, bestCost = INF;
        Pos bestBox = {-1, -1};
        char bestDir = 'S';

        int iterCount = 0;
        bool timedOut = false;
        for (const Pos& box : state.boxes) {
            if (timedOut) break;
            if (isBoxDeadlocked(state.grid, box)) continue;   // DEADLOCK-01: bỏ qua hộp đã kẹt

            for (int i = 0; i < 4; ++i) {
                if ((++iterCount % 8) == 0 && timer.istimeup()) { timedOut = true; break; }

                Pos dest  = {box.r + dr[i], box.c + dc[i]};
                Pos stand = {box.r - dr[i], box.c - dc[i]};

                if (!state.grid.isFree(dest) || state.hasBox(dest)) continue;
                if (state.grid.checkin(other) && dest == other) continue;
                if (!state.grid.isFree(stand) || state.hasBox(stand)) continue;
                if (state.grid.checkin(other) && stand == other) continue;

                char destCell = state.grid.cell[dest.r][dest.c];
                bool destIsGoal = (destCell == 'A' || destCell == 'B');
                if (!destIsGoal && isBoxDeadlocked(state.grid, dest)) continue;   // DEADLOCK-01

                int dBox  = min(distGoal[box.r][box.c],  DIST_CAP);
                int dDest = min(distGoal[dest.r][dest.c], DIST_CAP);
                int gain  = dBox - dDest;
                if (gain <= 0) continue;

                int cost = (agent == stand) ? 0 : distAgent[stand.r][stand.c];
                if (cost >= INF) continue;

                if (!found || gain > bestGain || (gain == bestGain && cost < bestCost)) {
                    found = true;
                    bestGain = gain;
                    bestCost = cost;
                    bestBox = box;
                    bestDir = DIRCH[i];
                }
            }
        }

        if (found) {
            mem.hasTarget  = true;
            mem.targetBox  = bestBox;
            mem.targetDir  = bestDir;
        } else {
            mem.hasTarget = false;
        }
    }

    // --- Thực thi commitment (cũ hoặc vừa chọn) ---
    char move = 'S';
    bool willPush = false;
    if (mem.hasTarget) {
        Pos box = mem.targetBox;
        auto [ddr, ddc] = getidx(mem.targetDir);
        Pos stand = {box.r - ddr, box.c - ddc};
        if (agent == stand) { move = mem.targetDir; willPush = true; }
        else move = firstStepTowards(state.grid, agent, stand, occ);
    }

    if (!canpush(state, agent, move)) { move = 'S'; willPush = false; }

    // Fallback (bổ sung sau A/B thật -- xem docs/eval_experiments.md Vòng 7):
    // nếu KHÔNG có commitment nào khả thi (mem.hasTarget vẫn false sau khi
    // vừa tìm), đừng đứng yên -- tiến về ô cạnh 1 hộp chưa deadlock, giống
    // fallback của botGreedy. Lý do: khi 2 bot tranh chấp đúng 1 hộp/1 ô
    // (cả 2 đều tất định, không tự có yếu tố ngẫu nhiên riêng), việc đứng
    // yên hoàn toàn dễ tạo dao động ổn định (bên này bị chặn -> đứng yên ->
    // bên kia lại đi vào đúng chỗ đó nữa -> lặp lại y hệt). Di chuyển dù nhỏ
    // cũng đủ phá vỡ tính đối xứng của vòng lặp.
    if (!mem.hasTarget && move == 'S') {
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
            char fbMove = firstStepTowards(state.grid, agent, target, occ);
            if (canpush(state, agent, fbMove)) move = fbMove;
        }
    }

    if (willPush) {
        // Cập nhật lạc quan vị trí hộp đang nhắm sang ô đích của cú đẩy này,
        // sẵn sàng cho lượt sau (tiếp tục đẩy hộp này nếu còn có lợi). Nếu
        // cú đẩy thực tế bị chặn bởi va chạm (resolveCollision), lượt sau
        // commitmentStillValid() sẽ phát hiện sai lệch (hộp không có ở vị
        // trí lạc quan này) và tự động tính lại mục tiêu -- không cần đồng
        // bộ ngược từ bên ngoài.
        auto [ddr, ddc] = getidx(mem.targetDir);
        Pos newBoxPos = {mem.targetBox.r + ddr, mem.targetBox.c + ddc};
        char destCell = state.grid.cell[newBoxPos.r][newBoxPos.c];
        if (destCell == 'A' || destCell == 'B') {
            mem.hasTarget = false;   // hộp sẽ ghi điểm và biến mất -> huỷ commitment
        } else {
            mem.targetBox = newBoxPos;
        }
    }

    return move;
}

// ---------------------------------------------------------------------------
// Tu: src/game/io_protocol.cpp
// ---------------------------------------------------------------------------
// ============================================================================
// IO PROTOCOL — module DUY NHẤT (cùng rules.cpp) được phép viết lại gần như
// hoàn toàn khi đề thật công bố định dạng chính thức. Mọi hàm khác trong
// project (core/, search/, evaluation/, bots/ phần logic) không phụ thuộc
// trực tiếp vào định dạng I/O cụ thể — chỉ nhận/trả gamestate.
//
// STDOUT LÀ KÊNH GIAO TIẾP VỚI CHƯƠNG TRÌNH GIÁM KHẢO.
// Không bao giờ in bất cứ gì ra cout ngoài đúng 1 ký tự nước đi.
// Mọi log/debug PHẢI đi qua cerr.
// ============================================================================

struct ParseResult {
    gamestate state;
    bool success = false;
    string errorMsg;   // chỉ dùng để ghi ra cerr khi debug, KHÔNG BAO GIỜ ra cout
};

// ----------------------------------------------------------------------------
// ASSUMPTION (INFERENCE, chưa phải FACT — xem THT2026_C1_Framework_Chuan_Bi.md
// Mục 2 "WHAT WE DON'T KNOW"): định dạng input hiện giả định là:
//
//   Dòng 1..16 : bàn cờ 16x16, mỗi dòng đúng 16 ký tự trong { . # A B a b X }
//   Dòng 17    : "<scoreA> <scoreB>" — điểm hiện tại của 2 đội, đọc thêm vì
//                hộp biến mất khỏi bàn sau khi ghi điểm nên không thể suy ra
//                điểm chỉ từ trạng thái bàn hiện tại (đã ghi rõ trong file
//                framework gốc, Mục 3, phần State).
//   Dòng 18+   : LỊCH SỬ (tuỳ chọn) — mỗi dòng "<moveA> <moveB>" của các lượt
//                đã qua, theo thứ tự thời gian. Đọc tới hết input hoặc tới
//                dòng rỗng. Đây là phần ít chắc chắn nhất, dựa trên FACT
//                "giao tiếp dự kiến: chương trình nhận trạng thái hiện tại +
//                lịch sử các lượt của hai đội".
//
// KHI CÓ ĐỀ THẬT: đây là hàm đầu tiên cần đối chiếu với docs/decision_tree.md
// nhánh "I/O khác với giả định hiện tại".
// ----------------------------------------------------------------------------
inline ParseResult parseInput(istream& in) {
    ParseResult result;
    try {
        vector<string> lines(16);
        for (int r = 0; r < 16; ++r) {
            if (!getline(in, lines[r])) {
                result.success = false;
                result.errorMsg = "Thieu dong ban co (chi doc duoc " + to_string(r) + "/16 dong)";
                return result;
            }
            // Cho phep dong co it hon 16 ky tu do CRLF/trailing space -> dem
            // bang '.' (an toan, khong crash) thay vi truy cap ngoai mang.
            while (lines[r].size() < 16) lines[r] += '.';
            if (lines[r].size() > 16) lines[r] = lines[r].substr(0, 16);
        }

        Grid raw = inp(lines);
        result.state = makeStateFromGrid(raw);

        // Dong 17: diem hien tai (neu co). Khong bat buoc -> khong co van la
        // hop le (vi du luot dau tien cua van, diem = 0-0 mac dinh).
        string scoreLine;
        if (getline(in, scoreLine) && !scoreLine.empty()) {
            istringstream iss(scoreLine);
            int sa = 0, sb = 0;
            if (iss >> sa >> sb) {
                result.state.scoreA = sa;
                result.state.scoreB = sb;
            }
            // Neu parse diem that bai: giu diem = 0-0 mac dinh, KHONG loi toan bo
            // parse (uu tien co duoc gamestate hop le hon la chinh xac tuyet doi
            // diem so o buoc nay).
        }

        // Dong 18+: lich su (tuy chon). TASK MERGE-03: TRUOC DAY doc het toi
        // EOF bang while(getline) -- dung duoc voi mo hinh exec-lai-moi-luot
        // (process thoat ngay sau do, khong quan trong con thua gi trong
        // stream). Nhung phat hien bug that khi them --interactive
        // (main.cpp): trong mo hinh process song, doc-het-toi-EOF nuot LUON
        // ca bang co cua LUOT SAU (khong co dau hieu nao phan biet "het lich
        // su luot nay" voi "bat dau luot ke tiep"). Cho toi khi HIST-01 dinh
        // nghia dinh dang lich su THAT (co so dong / dau ket thuc ro rang),
        // KHONG doc gi them o day -- an toan cho ca 2 mo hinh, dung mat gi
        // (lich su hien chua duoc luu/dung o dau ca, chi la stub).

        result.success = true;
        return result;
    } catch (const exception& e) {
        result.success = false;
        result.errorMsg = string("Exception khi parse input: ") + e.what();
        return result;
    } catch (...) {
        result.success = false;
        result.errorMsg = "Exception khong xac dinh khi parse input";
        return result;
    }
}

// Ban tien loi: parse tu 1 chuoi (dung cho test, khong doc tu stdin that).
inline ParseResult parseInputFromString(const string& text) {
    istringstream iss(text);
    return parseInput(iss);
}

// ----------------------------------------------------------------------------
// ASSUMPTION: output la dung 1 ky tu trong { U D L R S } roi xuong dong,
// khong kem gi khac. Neu de that yeu cau format khac (vi du JSON, hoac them
// toa do), day la ham CAN SUA DAU TIEN.
// ----------------------------------------------------------------------------
inline void formatOutput(ostream& out, char move) {
    out << move << '\n';
    out.flush();   // dam bao giam khao nhan duoc ngay, khong bi buffer giu lai
}

// Tao 1 gamestate rong an toan de dung lam fallback khi parse that bai hoan
// toan (vi du input trong hoac loi khong the phuc hoi). Mot ban do rong hoan
// toan (khong tuong, khong hop, agent o goc) luon hop le de goi bot len tren.
inline gamestate makeSafeEmptyState() {
    Grid g;
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c)
            g.cell[r][c] = '.';
    gamestate st;
    st.grid = g;
    st.a = {0, 0};
    st.b = {15, 15};
    st.boxes.clear();
    return st;
}

// ---------------------------------------------------------------------------
// Tu: main.cpp
// ---------------------------------------------------------------------------
// ============================================================================
// main.cpp — ENTRY POINT THI THẬT (TASK IO-02, roadmap.md Cổng 3).
//
// Đây KHÔNG phải chương trình demo/tournament nữa (bản cũ đã bị xoá, logic
// tournament nằm hẳn ở benchmarks/bench_bot_winrate.cpp — xem TASK ARCH-01).
//
// Hợp đồng (contract) của chương trình này:
//   input  (stdin)  -> 16 dòng bàn cờ + điểm hiện tại (xem io_protocol.cpp)
//   output (stdout) -> ĐÚNG 1 KÝ TỰ nước đi trong {U,D,L,R,S}, không kèm gì khác
//   exit code       -> luôn 0, không bao giờ crash dù input dị dạng
//
// ASSUMPTION (INFERENCE, xem THT2026_C1_Framework_Chuan_Bi.md Mục 2): chương
// trình có thể bị exec lại mỗi lượt ("các lần gọi (chạy) chương trình" — số
// nhiều). Do đó KHÔNG giữ gamestate sống trong RAM giữa các lần chạy —
// mỗi lần main() chạy là một lượt độc lập, đọc lại toàn bộ trạng thái từ
// stdin. Nếu đề thật xác nhận ngược lại (giữ 1 process sống xuyên game),
// đây là điểm đầu tiên cần sửa — xem docs/decision_tree.md.
// ============================================================================
// ASSUMPTION: time-limit mỗi lượt tính toán CHƯA ĐƯỢC CÔNG BỐ (xem Mục 2
// framework gốc). 900ms là HYPOTHESIS an toàn, chưa có cơ sở FACT. Khi đọc
// đề thật, đây là hằng số đầu tiên cần sửa. Trừ margin 20ms mặc định trong
// Timer -> bot thực tế dừng tính toán ở khoảng 880ms.
static const double TIME_LIMIT_MS = 900.0;

// Bot mặc định nộp bài: Bot 1 (Greedy/ShortestPush) — quyết định chính thức
// theo roadmap.md TASK LADDER-01: đây là bot mạnh nhất đã kiểm chứng
// (thắng Random 86.25%, thắng Heuristic 53.50% trên 200 ván, docs/eval_experiments.md
// Vòng 2). KHÔNG dùng Bot 3 (Heuristic) làm mặc định cho tới khi
// BOT3-COMMIT-01 (roadmap.md Cổng 5) chứng minh nó vượt Bot 1 bằng số liệu.
enum class BotChoice { GREEDY, HEURISTIC, RANDOM };
static const BotChoice DEFAULT_BOT = BotChoice::GREEDY;

// getsafemove — bọc mọi lời gọi bot bằng nhiều lớp phòng thủ:
//   1) Tính fallback (botRandom) trước tiên — luôn có sẵn.
//   2) Timer thật sự cắt được (đã sửa TASK ROBUST-01, không còn "chạy xong
//      hết mới hỏi hết giờ chưa" như bản cũ).
//   3) try/catch bọc toàn bộ logic chính — lỗi logic (không phải ASan/UBSan,
//      xem ghi chú trong roadmap.md P1-... về giới hạn của try/catch) rơi
//      về fallback thay vì làm crash toàn chương trình.
static char getsafemove(const gamestate& state, bool isa, mt19937& rng) {
    char fallback = 'S';
    try {
        fallback = botRandom(state, isa, rng);
    } catch (...) {
        fallback = 'S';
    }

    try {
        Timer timer(TIME_LIMIT_MS);
        switch (DEFAULT_BOT) {
            // STEP-06 (TIEBREAK-01): truyền &rng để Greedy có tie-break
            // ngẫu nhiên thật khi vào phòng thi, không chỉ trong benchmark.
            case BotChoice::GREEDY:    return botGreedy(state, isa, timer, &rng);
            case BotChoice::HEURISTIC: return botHeuristic(state, isa, timer);
            case BotChoice::RANDOM:    return botRandom(state, isa, rng);
        }
    } catch (...) {
        return fallback;
    }
    return fallback;
}

// TASK MERGE-03 (roadmap.md) — hỗ trợ CẢ 2 mô hình process, không đặt cược
// vào 1 giả định duy nhất:
//   (mặc định, không cờ) exec lại mỗi lượt — ASSUMPTION gốc của project này.
//   (--interactive)      process sống suốt ván, đọc lặp lại từ stdin — đúng
//   mô hình `Data/Judge/judge.cpp` (bạn cùng nhóm) THẬT SỰ dùng để test bot
//   của họ (`BotProcess::spawn(bot, "--interactive")`, giao tiếp qua pipe 2
//   chiều suốt ván, KHÔNG exec lại). Đây là bằng chứng độc lập thứ 2 (khác
//   nguồn với suy luận ban đầu từ "các lần gọi (chạy) chương trình") cho
//   khả năng đề thật dùng đúng mô hình này — đủ để chuẩn bị sẵn, không đủ
//   để COI LÀ FACT (họ tự đoán luật giống mình, không phải người ra đề).
//   Nếu đề thật xác nhận persistent-process: chỉ cần thêm "--interactive"
//   vào lệnh khởi chạy bot, không cần sửa dòng code nào khác.
static void runOneTurn(mt19937& rng) {
    ParseResult parsed = parseInput(cin);

    gamestate state;
    if (parsed.success) {
        state = parsed.state;
    } else {
        // Input dị dạng: không bao giờ để chương trình thoát mà không in gì.
        // Log lỗi ra cerr (KHÔNG BAO GIỜ ra cout — stdout là kênh giao tiếp
        // với giám khảo), rồi dùng 1 bàn cờ rỗng an toàn để vẫn trả 1 nước
        // đi hợp lệ (đứng yên).
        cerr << "[main] Parse input that bai: " << parsed.errorMsg << "\n";
        state = makeSafeEmptyState();
    }

    // ASSUMPTION: nhân vật của đội mình luôn là 'a' trong bàn cờ đọc được
    // (isa = true). Nếu đề thật phân biệt "mình"/"đối thủ" theo cách khác
    // (ví dụ tham số dòng lệnh riêng), sửa dòng này. LƯU Ý (Data/Judge):
    // judge.cpp của bạn cùng nhóm luôn gửi board đã "tự-quy-chiếu" (mình
    // luôn là 'a', đích của mình luôn là 'A', xem serialize_board(swapped=)
    // trong judge.cpp) — nếu đề thật làm giống vậy thì isa=true CHÍNH XÁC
    // trong MỌI trường hợp, kể cả khi thi đấu ở "vị trí B". Nếu đề thật gửi
    // board TUYỆT ĐỐI (không tự-quy-chiếu) thì dòng này cần đọc thêm 1 tham
    // số/dòng cho biết mình là 'a' hay 'b' thật.
    char move = getsafemove(state, /*isa=*/true, rng);

    formatOutput(cout, move);
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);

    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

    bool interactive = (argc >= 2 && string(argv[1]) == "--interactive");

    if (!interactive) {
        runOneTurn(rng);
        return 0;
    }

    // Vòng lặp sống suốt ván: dừng NGAY KHI stdin đã đóng sạch (EOF thật,
    // judge kết thúc ván bình thường) — không in gì thêm trong trường hợp
    // này, khớp đúng hành vi các bot trong Data/ (`if (!(cin >> line)) return 0;`).
    // Bất kỳ lỗi đọc nào KHÁC (input dị dạng giữa ván) vẫn phải trả 1 nước
    // đi (qua runOneTurn's fallback) để không làm lệch giao thức pipe.
    while (true) {
        cin.peek();
        if (cin.eof()) break;
        runOneTurn(rng);
    }
    return 0;
}
