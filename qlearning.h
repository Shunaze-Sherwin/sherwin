#pragma once
#include "common.h"

struct QEntry {
    array<double, 4> value{};
    int visits = 0;
};

unordered_map<unsigned long long, QEntry> q_table;
unsigned long long previous_key = 0;
int previous_action = -1;
int previous_score = 0;
int previous_enemy_score = 0;
bool has_previous_action = false;

unsigned long long state_key(const State &current) {
    unsigned long long key = 1469598103934665603ULL;
    auto add = [&](unsigned long long value) {
        key ^= value + 0x9e3779b97f4a7c15ULL + (key << 6) + (key >> 2);
    };
    add(current.me.first * 17 + current.me.second);
    add(current.enemy.first * 17 + current.enemy.second);
    add(current.my_score);
    add(current.enemy_score);
    for (const pair<int, int> &box : current.box)
        add(box.first * 17 + box.second);
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
    if (has_previous_action) {
        double reward = ((current.my_score - current.enemy_score) -
                 (previous_score - previous_enemy_score)) * 1000.0;
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
    has_previous_action = false;
}

void remember_action(const State &current, int action) {
    previous_key = state_key(current);
    previous_action = action;
    previous_score = current.my_score;
    previous_enemy_score = current.enemy_score;
    has_previous_action = true;
}

int learned_action(const State &current, int fallback) {
    QEntry &entry = q_table[state_key(current)];
    if (entry.visits < 3) return fallback;
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
