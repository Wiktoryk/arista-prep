#include <cassert>
#include <string>
#include "ring_buffer/ring_buffer.hpp"

int main() {
    {
        rb::SpscRingBuffer<int, 8> q;
        assert(q.empty());
        assert(!q.full());

        for (int i = 0; i < 7; ++i) {
            bool ok = q.emplace(i);
            assert(ok);
        }
        assert(q.full());

        int v = -1;
        for (int i = 0; i < 7; ++i) {
            bool ok = q.pop(v);
            assert(ok && v == i);
        }
        assert(q.empty());
    }

    {
        rb::SpscRingBuffer<std::string, 4> q;
        assert(!q.try_pop().has_value());
        q.push(std::string("hello"));
        auto x = q.try_pop();
        assert(x.has_value() && *x == "hello");
        assert(q.empty());
    }

    {
        rb::SpscRingBuffer<int, 2> q;
        assert(q.emplace(42));
        assert(!q.emplace(7));
        int v;
        assert(q.pop(v) && v == 42);
        assert(!q.pop(v));
    }

    return 0;
}
