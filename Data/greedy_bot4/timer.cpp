#pragma once
#include <bits/stdc++.h>
#include <chrono>
using namespace std;

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