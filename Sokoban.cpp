#include "common.h"
#include "calculate_distance.h"
#include "simulator.h"

signed main(){
    #define name "history"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }
    #undef name

    #define name "current_test"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }

    ios_base::sync_with_stdio(false);
    cin.tie(nullptr); cout.tie(nullptr);

//Load Input----------------------------------------------------------------------------------------
    int number_table = 1;
    //cin >> numbertable; fu(i, 1, number_table) Load_input();
    State current = Load_input();
    pre_hash_table();
//Simulator-----------------------------------------------------------------------------------------
    for (int tick = 1; tick < number_tick; tick += 2){
        chosen_move = {-1, -1};
        simulator(current, tick, tick);
        current = apply_move(current, chosen_move.first, 1);
        current = apply_move(current, chosen_move.second, 0);
        cout << "my move: " << command[chosen_move.first] << '\n';
        cout << "enemy move: " << command[chosen_move.second] << '\n';
    }
    cout << current.my_score << " " << current.enemy_score << endl;
}