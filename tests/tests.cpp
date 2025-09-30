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

    {
        rb::SpscRingBuffer<int, 2> q;
        assert(q.push(1));
        q.clear();
        assert(q.empty());
    }

    {
        rb::SpscRingBuffer<int, 2> q;
        assert(q.push(1));
        int v;
        assert(q.peek(v) && v == 1);
    }

    {
        rb::SpscRingBuffer<int, 4> q;
        for(int i{}; i < 3; i++) {
            assert(q.push(i));
        }
        int v1,v2;
        assert(q.pop_bulk(v1,v2) && v1 == 0 && v2 == 1);
    }

    {
        rb::SpscRingBuffer<int, 4> q;
        assert(q.emplace_bulk(1,2,3));
    }
    return 0;
}
