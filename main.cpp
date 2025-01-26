#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <ncurses.h>
#include <chrono>
#include <atomic>
#include <queue>

const int MIN_PHILOSOPHERS = 5;

std::vector<std::mutex> forks;
std::vector<std::condition_variable> cv;
std::vector<bool> fork_available;
std::vector<int> philosopher_state;
std::vector<int> philosopher_progress;
std::atomic<bool> running{true};

std::mutex queue_mutex;
std::queue<int> request_queue;
std::condition_variable queue_cv;

void philosopher(int id, int num_philosophers) {
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers;

    while (running) {
        philosopher_state[id] = 0;
        philosopher_progress[id] = 0;
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            philosopher_progress[id] = i * 10;
        }

        philosopher_state[id] = 1;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            request_queue.push(id);
            queue_cv.notify_all();
        }

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [&]() { return request_queue.front() == id; });
        }

        std::unique_lock<std::mutex> left_lock(forks[left_fork]);
        cv[left_fork].wait(left_lock, [&]() { return fork_available[left_fork]; });
        fork_available[left_fork] = false;

        std::unique_lock<std::mutex> right_lock(forks[right_fork]);
        cv[right_fork].wait(right_lock, [&]() { return fork_available[right_fork]; });
        fork_available[right_fork] = false;

        philosopher_state[id] = 2;
        philosopher_progress[id] = 0;
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            philosopher_progress[id] = i * 10;
        }

        fork_available[left_fork] = true;
        cv[left_fork].notify_one();
        fork_available[right_fork] = true;
        cv[right_fork].notify_one();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            request_queue.pop();
            queue_cv.notify_all();
        }
    }
}

void draw_philosophers(int num_philosophers) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

    while (running) {
        clear();
        for (int i = 0; i < num_philosophers; ++i) {
            mvprintw(i, 0, "Philosopher %d: ", i);
            if (philosopher_state[i] == 0) {
                attron(COLOR_PAIR(1));
                printw("Thinking [%d%%]", philosopher_progress[i]);
                attroff(COLOR_PAIR(1));
            } else if (philosopher_state[i] == 1) {
                attron(COLOR_PAIR(2));
                printw("Waiting for forks");
                attroff(COLOR_PAIR(2));
            } else if (philosopher_state[i] == 2) {
                attron(COLOR_PAIR(3));
                printw("Eating [%d%%]", philosopher_progress[i]);
                attroff(COLOR_PAIR(3));
            }
        }


        mvprintw(num_philosophers, 0, "Forks: ");
        for (int i = 0; i < num_philosophers; ++i) {
            if (fork_available[i]) {
                attron(COLOR_PAIR(4));
                printw("[F%d: FREE] ", i);
                attroff(COLOR_PAIR(4));
            } else {
                attron(COLOR_PAIR(5));
                printw("[F%d: BUSY] ", i);
                attroff(COLOR_PAIR(5));
            }
        }

        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        nodelay(stdscr, TRUE);
        char ch = getch();
        if (ch == 'q') {
            running = false;
        }
    }

    endwin();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number_of_philosophers>" << std::endl;
        return 1;
    }

    int num_philosophers = std::stoi(argv[1]);
    if (num_philosophers < MIN_PHILOSOPHERS) {
        std::cerr << "Number of philosophers must be at least " << MIN_PHILOSOPHERS << std::endl;
        return 1;
    }

    forks = std::vector<std::mutex>(num_philosophers);
    cv = std::vector<std::condition_variable>(num_philosophers);
    fork_available = std::vector<bool>(num_philosophers, true);
    philosopher_state = std::vector<int>(num_philosophers, 0);
    philosopher_progress = std::vector<int>(num_philosophers, 0);

    std::vector<std::thread> philosophers;
    for (int i = 0; i < num_philosophers; ++i) {
        philosophers.emplace_back(philosopher, i, num_philosophers);
    }

    std::thread drawer(draw_philosophers, num_philosophers);

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto& ph : philosophers) {
        ph.join();
    }
    drawer.join();

    return 0;
}