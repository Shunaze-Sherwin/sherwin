#include <bits/stdc++.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace std;

const int SIZE = 16;
const int dx[] = {0, 0, 1, -1};
const int dy[] = {1, -1, 0, 0};
const char command[] = {'R', 'L', 'D', 'U'};

struct Board {
    array<array<char, SIZE>, SIZE> cell{};
    pair<int, int> me{-1, -1};
    pair<int, int> enemy{-1, -1};
    int my_score = 0;
    int enemy_score = 0;
};

bool inside(int x, int y) {
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

Board read_board(const string &file_name) {
    Board board;
    ifstream input(file_name);
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            input >> board.cell[row][column];
            if (board.cell[row][column] == 'a') {
                board.me = {row, column};
                board.cell[row][column] = '.';
            }
            if (board.cell[row][column] == 'b') {
                board.enemy = {row, column};
                board.cell[row][column] = '.';
            }
        }
    }
    return board;
}

Board generate_board(const Board &layout, mt19937 &rng) {
    Board board = layout;
    vector<pair<int, int>> free_cells;
    vector<pair<pair<int, int>, char>> goals;

    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            char &cell = board.cell[row][column];
            if (cell == '#') continue;
            bool is_goal_cell = cell == 'A' || cell == 'B';
            if (is_goal_cell)
                goals.push_back({{row, column}, cell});
            else
                free_cells.push_back({row, column});
            cell = '.';
        }
    }

    shuffle(free_cells.begin(), free_cells.end(), rng);
    size_t next = 0;
    board.me = free_cells[next++];
    do {
        board.enemy = free_cells[next++];
    } while (board.enemy == board.me);

    for (const auto &goal : goals)
        board.cell[goal.first.first][goal.first.second] = goal.second;

    board.my_score = 0;
    board.enemy_score = 0;
    int boxes = static_cast<int>(goals.size());
    while (boxes > 0 && next < free_cells.size()) {
        auto box = free_cells[next++];
        if (box == board.me || box == board.enemy || board.cell[box.first][box.second] != '.')
            continue;
        board.cell[box.first][box.second] = 'X';
        --boxes;
    }
    return board;
}

void write_board(const Board &board, const string &file_name) {
    ofstream output(file_name);
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            char value = board.cell[row][column];
            if (board.me == make_pair(row, column)) value = 'a';
            if (board.enemy == make_pair(row, column)) value = 'b';
            output << value;
        }
        output << '\n';
    }
}

bool is_goal(char value) {
    return value == 'A' || value == 'B';
}

bool legal_step(const Board &board, bool mine, int direction) {
    auto position = mine ? board.me : board.enemy;
    int x = position.first + dx[direction];
    int y = position.second + dy[direction];
    if (!inside(x, y)) return false;
    auto other_position = mine ? board.enemy : board.me;
    if (make_pair(x, y) == other_position) return false;
    char next = board.cell[x][y];
    if (next == '.') return true;
    if (next != 'X') return false;
    int box_x = x + dx[direction];
    int box_y = y + dy[direction];
    if (!inside(box_x, box_y)) return false;
    if (make_pair(box_x, box_y) == other_position) return false;
    char destination = board.cell[box_x][box_y];
    return destination == '.' || is_goal(destination);
}

bool apply_move(Board &board, bool mine, int direction) {
    if (!legal_step(board, mine, direction)) return false;
    auto &position = mine ? board.me : board.enemy;
    int x = position.first + dx[direction];
    int y = position.second + dy[direction];
    if (board.cell[x][y] == 'X') {
        int box_x = x + dx[direction];
        int box_y = y + dy[direction];
        if (is_goal(board.cell[box_x][box_y])) {
            if (board.cell[box_x][box_y] == 'A') ++board.my_score;
            else ++board.enemy_score;
        } else {
            board.cell[box_x][box_y] = 'X';
        }
        board.cell[x][y] = '.';
    }
    position = {x, y};
    return true;
}

int decode_command(char value) {
    for (int direction = 0; direction < 4; ++direction)
        if (command[direction] == value) return direction;
    return -1;
}

string serialize_board(const Board &board, bool swapped = false) {
    ostringstream output;
    auto me = swapped ? board.enemy : board.me;
    auto enemy = swapped ? board.me : board.enemy;
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            char value = board.cell[row][column];
            if (me == make_pair(row, column)) value = 'a';
            if (enemy == make_pair(row, column)) value = 'b';
            output << value;
        }
        output << '\n';
    }
    output << (swapped ? board.enemy_score : board.my_score) << ' '
           << (swapped ? board.my_score : board.enemy_score) << '\n';
    return output.str();
}

class BotProcess {
public:
    pid_t pid = -1;
    int input = -1;
    int output = -1;

    explicit BotProcess(const char *bot) {
        int to_bot[2], from_bot[2];
        if (pipe(to_bot) || pipe(from_bot)) throw runtime_error("pipe failed");
        pid = fork();
        if (pid == 0) {
            dup2(to_bot[0], STDIN_FILENO);
            dup2(from_bot[1], STDOUT_FILENO);
            close(to_bot[0]); close(to_bot[1]);
            close(from_bot[0]); close(from_bot[1]);
            execl(bot, bot, "--interactive", (char *)nullptr);
            _exit(127);
        }
        close(to_bot[0]);
        close(from_bot[1]);
        input = to_bot[1];
        output = from_bot[0];
    }

    ~BotProcess() {
        if (input != -1) close(input);
        if (output != -1) close(output);
        if (pid != -1) {
            waitpid(pid, nullptr, 0);
        }
    }

    bool send_board(const Board &board) {
        string data = serialize_board(board);
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t count = write(input, data.data() + sent, data.size() - sent);
            if (count <= 0) return false;
            sent += count;
        }
        return true;
    }

    int receive_move() {
        pollfd descriptor{output, POLLIN, 0};
        if (poll(&descriptor, 1, 2000) <= 0) return -1;
        char value;
        int result = -1;
        do {
            if (read(output, &value, 1) != 1) return -1;
            if (result == -1) result = decode_command(value);
        } while (value != '\n');
        return result;
    }

    int move(const Board &board, bool swapped = false) {
        string data = serialize_board(board, swapped);
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t count = write(input, data.data() + sent, data.size() - sent);
            if (count <= 0) return -1;
            sent += count;
        }
        return receive_move();
    }
};

// Trước đây mỗi ván chỉ nhích goal/push đúng +-1 bất kể thắng đậm hay sát nút, nên tín
// hiệu từ một ván ăn may/xui vẫn tác động y hệt một ván áp đảo thực sự, dễ dao động qua
// lại quanh biên (100000 1000 500 5000 hiện đang kẹt ở trần 1000/500 từ trước). Giờ dùng
// trung bình động (EMA) của chênh lệch điểm để làm mượt nhiễu giữa các ván, rồi mới suy ra
// bước nhích theo đúng xu hướng gần đây thay vì phản ứng tức thời với 1 ván đơn lẻ.
void update_weights(int my_score, int enemy_score) {
    long long score, goal, push, dead_corner;
    double margin_ema;
    ifstream input("weights.dat");
    if (!(input >> score >> goal >> push >> dead_corner))
        score = 100000, goal = 100, push = 25, dead_corner = 5000;
    if (!(input >> margin_ema)) margin_ema = 0.0;

    margin_ema = 0.9 * margin_ema + 0.1 * (my_score - enemy_score);
    long long step = llround(max(-3.0, min(3.0, margin_ema)));

    goal = max(1LL, min(1000LL, goal + step));
    push = max(1LL, min(500LL, push + step));

    ofstream output("weights.dat");
    output << score << ' ' << goal << ' ' << push << ' ' << dead_corner << ' ' << margin_ema << '\n';
}

int random_legal_move(const Board &board, bool mine, mt19937 &rng) {
    vector<int> moves;
    for (int direction = 0; direction < 4; ++direction)
        if (legal_step(board, mine, direction)) moves.push_back(direction);
    if (moves.empty()) return 0;
    shuffle(moves.begin(), moves.end(), rng);
    return moves.front();
}

void print_board(const Board &board) {
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            char value = board.cell[row][column];
            if (board.me == make_pair(row, column)) value = 'a';
            if (board.enemy == make_pair(row, column)) value = 'b';
            cerr << value;
        }
        cerr << '\n';
    }
}

int human_move(const Board &board) {
    while (true) {
        cerr << "Your move (R/L/D/U): ";
        char value;
        if (!(cin >> value)) return -1;
        value = static_cast<char>(toupper(static_cast<unsigned char>(value)));
        int direction = decode_command(value);
        if (direction >= 0 && legal_step(board, false, direction))
            return direction;
        cerr << "Invalid move. Try again.\n";
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: ./judge <bot-a> [bot-b] [games] [ticks]\\n";
        return 1;
    }
    bool second_bot_given = argc >= 3 && string(argv[2]) != "human" &&
                            !isdigit(static_cast<unsigned char>(argv[2][0]));
    const char *second_bot = second_bot_given ? argv[2] : nullptr;
    int games = second_bot_given ? (argc >= 4 ? atoi(argv[3]) : 100)
                                 : (argc >= 3 ? atoi(argv[2]) : 100);
    int ticks = second_bot_given ? (argc >= 5 ? atoi(argv[4]) : 50)
                                 : (argc >= 4 ? atoi(argv[3]) : 50);
    bool human = !second_bot_given && argc >= 5 && string(argv[4]) == "human";
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    Board initial = read_board("current_test.INP");
    BotProcess bot_a(argv[1]);
    unique_ptr<BotProcess> bot_b;
    if (second_bot) bot_b = make_unique<BotProcess>(second_bot);
    ofstream log("training.log", ios::app);
    vector<tuple<int, int, int>> results;
    int wins = 0;
    int draws = 0;
    int losses = 0;

    for (int game = 0; game < games; ++game) {
        Board board = generate_board(initial, rng);
        for (int tick = 0; tick < ticks; ++tick) {
            if (human) {
                cerr << "\nGame " << game + 1 << ", tick " << tick + 1 << '\n';
                print_board(board);
            }
            int my_move;
            int enemy_move;
            if (human) {
                if (!bot_a.send_board(board)) {
                    cerr << "bot input pipe closed\n";
                    break;
                }
                enemy_move = human_move(board);
                my_move = bot_a.receive_move();
            } else if (bot_b) {
                my_move = bot_a.move(board);
                enemy_move = bot_b->move(board, true);
            } else {
                my_move = bot_a.move(board);
                enemy_move = random_legal_move(board, false, rng);
            }
              if (my_move < 0) {
                 cerr << "game " << game + 1 << ", tick " << tick
                     << ": bot did not return a valid command\n";
                 break;
              }
            if (enemy_move < 0) break;
            apply_move(board, true, my_move);
            apply_move(board, false, enemy_move);
              cerr << "game " << game + 1 << ", tick " << tick + 1
                  << ": sent board, bot=" << command[my_move]
                  << ", enemy=" << command[enemy_move] << '\n';
            log << game << ' ' << tick << ' ' << board.my_score << ' '
                << board.enemy_score << ' ' << command[my_move] << ' '
                << command[enemy_move] << '\n';
        }
        cerr << "game " << game + 1 << "/" << games << ": "
             << board.my_score << '-' << board.enemy_score << '\n';
           results.emplace_back(game + 1, board.my_score, board.enemy_score);
           if (board.my_score > board.enemy_score) ++wins;
           else if (board.my_score == board.enemy_score) ++draws;
           else ++losses;
        update_weights(board.my_score, board.enemy_score);
    }
            cout << "\n+--------+----------+----------+--------+\n";
            cout << "| Game   | Bot A    | Bot B    | Result |\n";
            cout << "+--------+----------+----------+--------+\n";
        for (const auto &[game, score_a, score_b] : results) {
           string result = score_a > score_b ? "WIN" :
                        score_a < score_b ? "LOSS" : "DRAW";
               cout << "| " << left << setw(6) << game
                   << " | " << setw(8) << score_a
                   << " | " << setw(8) << score_b
                   << " | " << setw(6) << result << " |\n";
        }
            cout << "+--------+----------+----------+--------+\n";
            cout << "\n+--------+--------+--------+--------+\n";
            cout << "| Bot    | Wins   | Draws  | Losses |\n";
            cout << "+--------+--------+--------+--------+\n";
            cout << "| Bot A  | " << left << setw(6) << wins
                << " | " << setw(6) << draws << " | " << setw(6) << losses << " |\n";
            cout << "| Bot B  | " << left << setw(6) << losses
                << " | " << setw(6) << draws << " | " << setw(6) << wins << " |\n";
            cout << "+--------+--------+--------+--------+\n";
}
