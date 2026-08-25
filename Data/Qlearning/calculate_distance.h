#pragma once
#include "common.h"

bool safe(int x, int y, const State &current){
    return current.get(x, y) == 1;
}

// quality() gọi bfs() 2 LẦN cho mỗi trạng thái lá trong minimax, nên chi phí của
// riêng hàm này cộng dồn rất lớn qua cả cây tìm kiếm. std::queue<pair<int,int>> mặc
// định dùng std::deque bên dưới, cấp phát heap theo từng khối bộ nhớ khi push - thay
// bằng mảng tĩnh + 2 con trỏ đầu/cuối: BFS trên lưới 16x16 duyệt tối đa 256 ô (mỗi ô
// chỉ được đẩy vào hàng đợi đúng 1 lần nhờ điều kiện cost[x][y] != -1), nên mảng 256
// phần tử luôn đủ chỗ - không có rủi ro tràn, khác với số hộp (không có giới hạn cứng
// trong luật chơi) nên chỗ đó vẫn cần BoxList có heap fallback thay vì mảng tĩnh thuần.
array<array<int, 17>, 17> bfs(pair<int, int> start, const State &current){
    array<array<int, 17>, 17> cost;
    fu(i, 1, 16) fu(j, 1, 16) cost[i][j] = -1;
    array<pair<int, int>, 256> queue_buffer;
    int head = 0, tail = 0;

    queue_buffer[tail++] = start;
    cost[start.first][start.second] = 0;

    while (head < tail){
        pair<int, int> tmp = queue_buffer[head++];
        fu(i, 0, 3){
            int x = tmp.first + dx[i];
            int y = tmp.second + dy[i];
            if (!safe(x, y, current) || cost[x][y] != -1) continue;
            cost[x][y] = cost[tmp.first][tmp.second] + 1;
            queue_buffer[tail++] = {x, y};
        }
    }
    return cost;
}