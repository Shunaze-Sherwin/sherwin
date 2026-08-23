#pragma once
#include <bits/stdc++.h>

using namespace std;

typedef long long ll;

#define fu(i, a, b) for (int i = (a); i <= (b); ++i)
#define fd(i, a, b) for (int i = (a); i >= (b); --i)

template<typename T> inline bool minimize(T &a, const T &b){return a > b ? a = b, 1 : 0;}
template<typename T> inline bool maximize(T &a, const T &b){return a < b ? a = b, 1 : 0;}

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

ll Rand(ll l, ll r){
    return l + rng() % (r - l + 1);
}

string command[] = {"R", "L", "D", "U"};
int dx[] = {0, 0, 1, -1};
int dy[] = {1, -1, 0, 0};

// BoxList: mảng sức chứa cố định (mặc định 16 ô) thay cho vector<pair<int,int>> để
// lưu danh sách hộp trong State. State được copy-by-value RẤT nhiều lần trong minimax
// (apply_move nhận/trả State theo giá trị, generate_moves tạo tới 4 bản mỗi nút, mỗi
// nút của cây tìm kiếm gọi generate_moves 2 lần) - với vector, MỖI lần copy đó là 1
// lần cấp phát heap (new[]) + giải phóng (delete[]), đắt hơn hẳn so với copy mảng
// tĩnh (chỉ là memcpy vài chục byte). Bản đồ hiện tại chỉ có 3-5 hộp nên 16 đã dư dả;
// nếu bản đồ tương lai có nhiều hộp hơn sức chứa, tự động chuyển sang cấp phát heap
// (giống hệt vector) để KHÔNG BAO GIỜ tràn bộ nhớ - chỉ chậm lại đúng bằng vector ở
// trường hợp hiếm đó, không đánh đổi lấy rủi ro đúng-sai.
template <typename T, int N>
struct BoxList {
    array<T, N> inline_data{};
    unique_ptr<T[]> heap_data;
    int capacity = N;
    int count = 0;

    T* data() { return heap_data ? heap_data.get() : inline_data.data(); }
    const T* data() const { return heap_data ? heap_data.get() : inline_data.data(); }

    BoxList() = default;
    BoxList(const BoxList &other) : inline_data(other.inline_data), capacity(other.capacity), count(other.count) {
        if (other.heap_data) {
            heap_data = make_unique<T[]>(capacity);
            copy(other.data(), other.data() + count, heap_data.get());
        }
    }
    BoxList& operator=(const BoxList &other) {
        if (this == &other) return *this;
        if (other.heap_data) {
            heap_data = make_unique<T[]>(other.capacity);
            capacity = other.capacity;
            copy(other.data(), other.data() + other.count, heap_data.get());
        } else {
            heap_data.reset();
            capacity = N;
            inline_data = other.inline_data;
        }
        count = other.count;
        return *this;
    }
    // Tự viết move thay vì "= default" để đối tượng bị move-khỏi luôn còn ở trạng
    // thái rỗng hợp lệ (count = 0), tránh đọc nhầm dữ liệu cũ còn sót trong
    // inline_data nếu lỡ có chỗ nào đọc lại đối tượng đã bị move (hiện tại không có,
    // nhưng đây là chi phí gần như 0 để bảo đảm an toàn tuyệt đối).
    BoxList(BoxList &&other) noexcept
        : inline_data(other.inline_data), heap_data(std::move(other.heap_data)),
          capacity(other.capacity), count(other.count) {
        other.count = 0;
        other.capacity = N;
    }
    BoxList& operator=(BoxList &&other) noexcept {
        if (this == &other) return *this;
        inline_data = other.inline_data;
        heap_data = std::move(other.heap_data);
        capacity = other.capacity;
        count = other.count;
        other.count = 0;
        other.capacity = N;
        return *this;
    }

    int size() const { return count; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    T* begin() { return data(); }
    T* end() { return data() + count; }
    const T* begin() const { return data(); }
    const T* end() const { return data() + count; }

    void push_back(const T &value) {
        if (count == capacity) grow();
        data()[count++] = value;
    }

    // Xoá theo con trỏ (kiểu iterator) để giữ nguyên cách gọi hiện có ở simulator.h
    // (current.box.erase(current.box.begin() + pos)) - không cần sửa call site. Dùng
    // swap-với-phần-tử-cuối (O(1)) thay vì dồn mảng (O(n)) vì thứ tự hộp không mang ý
    // nghĩa gì trong toàn bộ codebase: hash_vector dùng XOR (không phụ thuộc thứ tự),
    // mọi nơi khác duyệt độc lập từng hộp hoặc tìm hộp theo TOẠ ĐỘ chứ không theo chỉ số.
    void erase(T *it) {
        int index = static_cast<int>(it - data());
        data()[index] = data()[--count];
    }

   private:
    void grow() {
        int new_capacity = capacity * 2;
        unique_ptr<T[]> new_data = make_unique<T[]>(new_capacity);
        copy(data(), data() + count, new_data.get());
        heap_data = move(new_data);
        capacity = new_capacity;
    }
};

//State---------------------------------------------------------------------------------------
int target[17][17];
bool wall[17][17];
struct State{
    array<unsigned long long, 4> table{};
    pair<int, int> me;
    pair<int, int> enemy;
    BoxList<pair<int, int>, 16> box;
    int my_score = 0;
    int enemy_score = 0;

    void update(int x, int y, int val){
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        if (val) table[x] |= (1ull << y);
        else table[x] &= ~(1ull << y);
    }

    int get(int x, int y) const{
        if (x < 1 || x > 16 || y < 1 || y > 16) return 0;
        if (x == enemy.first && y == enemy.second) return 0;
        if (x == me.first && y == me.second) return 0;

        if (target[x][y] == 1) return 2;
        if (target[x][y] == -1) return -2;
        y = (x - 1) % 4 * 16 + y - 1;
        x = (x - 1) / 4;
        return (table[x] >> y) & 1ull;
    }
    // 0: wall or something on it
    // 1: empty
    // 2: my point
    // -2: enemy's point
};

bool Load_input(State &res){
    res = State{};
    memset(target, 0, sizeof(target));
    memset(wall, 0, sizeof(wall));
    fu(i, 1, 16) fu(j, 1, 16) {
        char c;
        if (!(cin >> c)) return false;
        if (c == 'a') res.me = {i, j};
        if (c == 'b') res.enemy = {i, j};
        if (c == 'A') target[i][j] = 1;
        if (c == 'B') target[i][j] = -1;
        if (c == 'X') res.box.push_back({i, j});
        if (c == '#') wall[i][j] = 1;
        if (c != '#' && c != 'X' && c != 'A' && c != 'B') res.update(i, j, 1);
    }
    int my_score, enemy_score;
    if (cin >> my_score >> enemy_score){
        res.my_score = my_score;
        res.enemy_score = enemy_score;
    }
    return true;
}

State Load_input(){
    State result;
    Load_input(result);
    return result;
}

bool is_wall(int x, int y){
    if (x < 1 || y < 1 || x > 16 || y > 16) return true;
    return wall[x][y];
}

int find_box(const State &current, int x, int y){
    fu(i, 0, (int)current.box.size() - 1) if (current.box[i] == make_pair(x, y)) return i;
    return -1;
}

//Hash State---------------------------------------------------------------------------------
ll code[17][17];

void pre_hash_table(){
    fu(i, 1, 16) fu(j, 1, 16) code[i][j] = Rand(0, 1e14);
}

ll hash_vector(const BoxList<pair<int, int>, 16> &carry){
    ll res = 0;
    for (pair<int, int> tmp : carry) res ^= code[tmp.first][tmp.second];
    return res;
}

array<ll, 4> hash_table(const State &current){
    array<ll, 4> res = {0, 0, 0, 0};
    res[0] = current.me.first * 100 + current.me.second;
    res[1] = current.enemy.first * 100 + current.enemy.second;
    res[2] = hash_vector(current.box);
    res[3] = current.my_score * 1000 + current.enemy_score;

    return res;
}