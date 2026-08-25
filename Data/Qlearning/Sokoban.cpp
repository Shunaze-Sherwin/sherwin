#include "common.h"
#include "simulator.h"
#include "qlearning.h"

signed main(int argc, char **argv){
    bool interactive = argc > 1 && string(argv[1]) == "--interactive";
    if (!interactive) {
    #define name "history"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }
    }
    #undef name

    if (!interactive) {
        #define name "current_test"
        if (fopen(name".INP", "r")){
            freopen(name".INP", "r", stdin);
            freopen(name".OUT", "w", stdout);
        }
        #undef name
    }

    ios_base::sync_with_stdio(false);
    cin.tie(nullptr); cout.tie(nullptr);
    load_q_table();

//Load Input----------------------------------------------------------------------------------------
    //int number_table = 1; cin >> numbertable; fu(i, 1, number_table) Load_input();
    while (true){
        State current;
        if (!Load_input(current)) break;
        pre_hash_table();
        init_quality(current);
        learn_state(current);
        chosen_move = {-1, -1};
        search_deadline = chrono::steady_clock::now() + SEARCH_TIME_BUDGET;
        run_search(current);
        // learned_action() có thể gợi ý một nước khác với minimax - nhưng nếu nước đó
        // ĐẢO NGƯỢC last_move, bỏ qua gợi ý và giữ lựa chọn của minimax (đã có
        // REVERSAL_PENALTY chống đảo-ngược ở run_search()). Q-table dùng state_key()
        // dạng tương đối/kẹp (clamp) nên nhiều vị trí thật khác nhau có thể băm trùng
        // key, khiến 2 "bucket" học được hành động trỏ vòng qua lại nhau (A->B, B->A) -
        // không kiểm tra ở đây thì override sẽ phá luôn REVERSAL_PENALTY và gây lặp vô hạn.
        int learned = learned_action(current, chosen_move.first);
        if (!is_reverse_move(learned, last_move)) chosen_move.first = learned;
        if (chosen_move.first == -1) cout << "S\n";
        else cout << command[chosen_move.first] << '\n';
        cout.flush();
        last_move = chosen_move.first;
        if (chosen_move.first != -1) remember_action(current, chosen_move.first);
        if (!interactive) break;
    }
    save_q_table();
}