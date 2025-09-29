#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "ring_buffer/ring_buffer.hpp"

int main() {
    rb::SpscRingBuffer<int, 1024> q;

    std::thread prod([&]{
        for (int i = 0; i < 10'000; ++i) {
            while (!q.emplace(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread cons([&]{
        int value{};
        std::size_t count = 0;
        while (count < 10'000) {
            if (q.pop(value)) {
                if (value % 2500 == 0) {
                    std::cout << "got " << value << "\n";
                }
                ++count;
            } else {
                std::this_thread::yield();
            }
        }
    });

    prod.join();
    cons.join();
    std::cout << "done\n";
    return 0;
}
