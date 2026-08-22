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
        learn_state(current);
        pre_hash_table();
        init_quality(current);
        simulator(current, 1, 1);
        chosen_move.first = learned_action(current, chosen_move.first);
        if (chosen_move.first == -1) cout << "S\n";
        else cout << command[chosen_move.first] << '\n';
        cout.flush();
        if (chosen_move.first != -1) remember_action(current, chosen_move.first);
        if (!interactive) break;
    }
    save_q_table();
}