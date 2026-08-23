#pragma once
#include "common.h"
#include "quality.h"

struct QEntry {
    array<double, 4> value{};
    int visits = 0;
};

unordered_map<unsigned long long, QEntry> q_table;
unsigned long long previous_key = 0;
int previous_action = -1;
int previous_score = 0;
int previous_enemy_score = 0;
ll previous_quality = 0;
bool has_previous_action = false;

// state_key trước đây băm theo tọa độ TUYỆT ĐỐI (vị trí, từng hộp) trên bản đồ 16x16.
// Vì bản đồ được xáo ngẫu nhiên mỗi ván (generate_board trong judge.cpp), gần như
// không trạng thái nào lặp lại giữa các ván -> visits luôn < 3 -> learned_action() gần
// như không bao giờ được dùng. Ở đây đổi sang đặc trưng TƯƠNG ĐỐI: lệch tọa độ (dr, dc)
// từ mình tới hộp có lượt-đẩy-còn-thiếu (distance_to_push) ít nhất, cộng hiệu điểm và số
// hộp còn lại, đều được rút gọn (bucket) về khoảng nhỏ. (dr, dc) vẫn là độ lệch tọa độ
// thật nên hướng R/L/D/U học được vẫn đúng ý nghĩa trên MỌI bản đồ, không chỉ ván đã gặp.
int clampDelta(int value, int limit) {
    return max(-limit, min(limit, value));
}

unsigned long long state_key(const State &current) {
    pair<int, int> best_box = {0, 0};
    int best_push = (int)1e9;
    int best_direction = 4;
    for (const pair<int, int> &box : current.box) {
        int d = distance_to_push(current, box, true);
        if (d < best_push) {
            best_push = d;
            best_box = box;
            fu(direction, 0, 3) {
                int next_x = box.first + dx[direction];
                int next_y = box.second + dy[direction];
                int stand_x = box.first - dx[direction];
                int stand_y = box.second - dy[direction];
                if (next_x >= 1 && next_x <= 16 && next_y >= 1 && next_y <= 16 &&
                    stand_x >= 1 && stand_x <= 16 && stand_y >= 1 && stand_y <= 16 &&
                    goal_dist[0][next_x][next_y] == goal_dist[0][box.first][box.second] - 1)
                    best_direction = direction;
            }
        }
    }

    int dr = 0, dc = 0;
    if (best_push < (int)1e9) {
        dr = clampDelta(best_box.first - current.me.first, 6);
        dc = clampDelta(best_box.second - current.me.second, 6);
    }
    int score_diff = clampDelta(current.my_score - current.enemy_score, 4);
    int box_count = min((int)current.box.size(), 4);
    int enemy_dr = clampDelta(current.enemy.first - current.me.first, 6);
    int enemy_dc = clampDelta(current.enemy.second - current.me.second, 6);
    int goal_distance = best_push < (int)1e9
        ? clampDelta(goal_dist[0][best_box.first][best_box.second], 16) : 16;

    unsigned long long key = 1469598103934665603ULL ^ 0x514c7632ULL;
    auto add = [&](unsigned long long value) {
        key ^= value + 0x9e3779b97f4a7c15ULL + (key << 6) + (key >> 2);
    };
    add((unsigned long long)(dr + 6));
    add((unsigned long long)(dc + 6));
    add((unsigned long long)(score_diff + 4));
    add((unsigned long long)box_count);
    add((unsigned long long)(best_direction + 1));
    add((unsigned long long)(enemy_dr + 6));
    add((unsigned long long)(enemy_dc + 6));
    add((unsigned long long)(goal_distance + 16));
    return key;
}

void load_q_table() {
    ifstream input("qtable.dat");
    unsigned long long key;
    QEntry entry;
    while (input >> key >> entry.visits >> entry.value[0] >> entry.value[1]
                >> entry.value[2] >> entry.value[3])
        q_table[key] = entry;
}

void save_q_table() {
    ofstream output("qtable.dat");
    output << setprecision(12);
    for (const auto &item : q_table) {
        output << item.first << ' ' << item.second.visits;
        for (double value : item.second.value) output << ' ' << value;
        output << '\n';
    }
}

void learn_state(const State &current) {
    unsigned long long key = state_key(current);
    if (has_previous_action &&
        (current.my_score < previous_score || current.enemy_score < previous_enemy_score))
        has_previous_action = false;
    if (has_previous_action) {
        ll score_delta = (current.my_score - current.enemy_score) -
                         (previous_score - previous_enemy_score);
        ll current_quality = quality(current);
        ll quality_delta = current_quality - previous_quality;
        quality_delta = max(-1000LL, min(1000LL, quality_delta / 100));
        double reward = score_delta * 1000.0 + quality_delta;
        QEntry &previous = q_table[previous_key];
        QEntry &next = q_table[key];
        double best_next = *max_element(next.value.begin(), next.value.end());
        constexpr double alpha = 0.15;
        constexpr double gamma = 0.90;
        previous.value[previous_action] += alpha *
            (reward + gamma * best_next - previous.value[previous_action]);
        ++previous.visits;
    }
    previous_key = key;
    previous_score = current.my_score;
    previous_enemy_score = current.enemy_score;
    previous_quality = quality(current);
    has_previous_action = false;
}

void remember_action(const State &current, int action) {
    previous_key = state_key(current);
    previous_action = action;
    previous_score = current.my_score;
    previous_enemy_score = current.enemy_score;
    previous_quality = quality(current);
    has_previous_action = true;
}

int learned_action(const State &current, int fallback) {
    QEntry &entry = q_table[state_key(current)];
    // FIX: 96.6% cac trang thai dat visits>=3 trong qtable.dat hien tai van
    // co CA 4 gia tri Q = 0 (chua he hoc duoc gi cho trang thai do - chi la
    // "ghe qua" nhieu lan ma chua bao gio nhan duoc thuong khac 0 de cap
    // nhat). Neu chi xet visits>=3 nhu truoc, ham nay se "tin" vao 4 so 0
    // bang nhau va CHON LIEU 1 huong (theo thu tu R,L,D,U) - co the ghi de
    // mot nuoc minimax tot bang 1 nuoc chua hoc gi ca. Them dieu kien: phai
    // co it nhat 1 gia tri Q khac 0 thi moi coi la "da hoc" va dung de ghi
    // de fallback.
    bool has_signal = false;
    for (double v : entry.value) if (v != 0.0) { has_signal = true; break; }
    if (entry.visits < 3 || !has_signal) return fallback;
    int best_action = fallback;
    double best_value = -numeric_limits<double>::infinity();
    fu(action, 0, 3) {
        State next = apply_move(current, action, true);
        if (hash_table(next) == hash_table(current)) continue;
        if (entry.value[action] > best_value) {
            best_value = entry.value[action];
            best_action = action;
        }
    }
    return best_action;
}