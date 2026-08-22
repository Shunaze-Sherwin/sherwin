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

string serialize_board(const Board &board) {
    ostringstream output;
    for (int row = 0; row < SIZE; ++row) {
        for (int column = 0; column < SIZE; ++column) {
            char value = board.cell[row][column];
            if (board.me == make_pair(row, column)) value = 'a';
            if (board.enemy == make_pair(row, column)) value = 'b';
            output << value;
        }
        output << '\n';
    }
    output << board.my_score << ' ' << board.enemy_score << '\n';
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

    int move(const Board &board) {
        if (!send_board(board)) return -1;
        return receive_move();
    }
};

void update_weights(int my_score, int enemy_score) {
    long long score, goal, push, dead_corner;
    ifstream input("weights.dat");
    if (!(input >> score >> goal >> push >> dead_corner))
        score = 100000, goal = 100, push = 25, dead_corner = 5000;

    if (my_score > enemy_score) {
        goal = min(1000LL, goal + 1);
        push = min(500LL, push + 1);
    } else if (my_score < enemy_score) {
        goal = max(1LL, goal - 1);
        push = max(1LL, push - 1);
    }
    ofstream output("weights.dat");
    output << score << ' ' << goal << ' ' << push << ' ' << dead_corner << '\n';
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
        cerr << "Usage: ./judge <bot-executable> [games] [ticks]\\n";
        return 1;
    }
    int games = argc >= 3 ? atoi(argv[2]) : 100;
    int ticks = argc >= 4 ? atoi(argv[3]) : 50;
    bool human = argc >= 5 && string(argv[4]) == "human";
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    Board initial = read_board("current_test.INP");
    BotProcess bot(argv[1]);
    ofstream log("training.log", ios::app);
    int wins = 0;

    for (int game = 0; game < games; ++game) {
        Board board = initial;
        for (int tick = 0; tick < ticks; ++tick) {
            if (human) {
                cerr << "\nGame " << game + 1 << ", tick " << tick + 1 << '\n';
                print_board(board);
            }
            int my_move;
            int enemy_move;
            if (human) {
                if (!bot.send_board(board)) {
                    cerr << "bot input pipe closed\n";
                    break;
                }
                enemy_move = human_move(board);
                my_move = bot.receive_move();
            } else {
                my_move = bot.move(board);
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
            if (board.my_score > board.enemy_score) ++wins;
            log << game << ' ' << tick << ' ' << board.my_score << ' '
                << board.enemy_score << ' ' << command[my_move] << ' '
                << command[enemy_move] << '\n';
        }
        cerr << "game " << game + 1 << "/" << games << ": "
             << board.my_score << '-' << board.enemy_score << '\n';
        update_weights(board.my_score, board.enemy_score);
    }
    cout << "completed " << games << " games, leading ticks: " << wins << '\n';
}
