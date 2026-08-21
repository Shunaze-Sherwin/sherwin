#include <bits/stdc++.h>

using namespace std;

#define fu(i, a, b) for (int i = (a); i <= (b); ++i)
#define fd(i, a, b) for (int i = (a); i >= (b); --i)

struct State{
    int table[17][17];
    pair<int, int> me;
    pair<int, int> enemy;
};

void Load_input(){
    fu(i, 1, 16) {
        fu(j, 1, 16) cout << '.';
        cout << '\n';
    }

}

signed main(){
    #define name "history"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }

    #define name "current_test"
    if (fopen(name".INP", "r")){
        freopen(name".INP", "r", stdin);
        freopen(name".OUT", "w", stdout);
    }

    ios_base::sync_with_stdio(false);
    cin.tie(nullptr); cout.tie(nullptr);

    int number_table = 1;
    //cin >> numbertable;
    fu(i, 1, number_table) Load_input();


}