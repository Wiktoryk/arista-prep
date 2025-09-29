#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "ring_buffer/ring_buffer.hpp"

using hiresclock_t = std::chrono::high_resolution_clock;

int main() {
    constexpr std::size_t N = 5'000'000;
    rb::SpscRingBuffer<uint64_t, 1 << 14> q;

    auto t0 = hiresclock_t::now();
    std::thread prod([&] {
        for (std::size_t i = 0; i < N; ++i) {
            while (!q.emplace(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread cons([&] {
        std::size_t seen = 0;
        uint64_t v;
        while (seen < N) {
            if (q.pop(v)) {
                ++seen;
            }
        }
    });

    prod.join();
    cons.join();
    auto t1 = hiresclock_t::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double mops = (double)N / (double)ms / 1000.0;
    std::cout << "Transferred " << N << " items in " << ms << " ms (" << mops << " Mops)\n";
    return 0;
}
