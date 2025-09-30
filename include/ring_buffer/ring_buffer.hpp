#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <optional>

// Fixed-size lock-free SPSC ring buffer.
// Requirements:
//  - Capacity must be a power of two (for fast masking).
//  - Single producer thread calls push()/emplace().
//  - Single consumer thread calls pop()/try_pop().
//  - T must be trivially moveable or at least movable; copy works too.

namespace rb {

constexpr bool is_power_of_two(std::size_t x) {
    return x && ((x & (x - 1)) == 0);
}

template <typename T, std::size_t CapacityPow2>
class SpscRingBuffer {
    static_assert(is_power_of_two(CapacityPow2), "CapacityPow2 must be a power of two");
    static_assert(CapacityPow2 >= 2, "CapacityPow2 must be >= 2");
    using self_t = SpscRingBuffer<T, CapacityPow2>;

    static constexpr std::size_t Capacity = CapacityPow2;
    static constexpr std::size_t Mask = CapacityPow2 - 1;

public:
    SpscRingBuffer() : head(0), tail(0) {}

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

    bool push(const T& v) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        return emplace(v);
    }
    bool push(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>) {
        return emplace(std::move(v));
    }

    template <class... Args>
    bool emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        const auto head_loaded = head.load(std::memory_order_relaxed);
        const auto next = head_loaded + 1;
        if (next - tail.load(std::memory_order_acquire) > Capacity - 1) {
            return false;
        }
        std::size_t idx = (head_loaded & Mask);
        auto* slot = reinterpret_cast<T*>(&storage[idx * sizeof(T)]);
        std::construct_at(slot, std::forward<Args>(args)...);
        head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
        const auto tail_loaded = tail.load(std::memory_order_relaxed);
        if (tail_loaded == head.load(std::memory_order_acquire)) {
            return false;
        }
        std::size_t idx = (tail_loaded & Mask);
        auto* slot = reinterpret_cast<T*>(&storage[idx * sizeof(T)]);
        out = std::move(*slot);
        std::destroy_at(slot);
        tail.store(tail_loaded + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> try_pop() noexcept(std::is_nothrow_move_constructible_v<T>) {
        const auto tail_loaded = tail.load(std::memory_order_relaxed);
        if (tail_loaded == head.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        std::size_t idx = (tail_loaded & Mask);
        auto* slot = reinterpret_cast<T*>(&storage[idx * sizeof(T)]);
        std::optional<T> ret{std::move(*slot)};
        std::destroy_at(slot);
        tail.store(tail_loaded + 1, std::memory_order_release);
        return ret;
    }

    [[nodiscard]] bool empty() const noexcept {
        return tail.load(std::memory_order_acquire) == head.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool full() const noexcept {
        return (head.load(std::memory_order_acquire) - tail.load(std::memory_order_acquire)) >= (Capacity - 1);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return head.load(std::memory_order_acquire) - tail.load(std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    void clear() noexcept(noexcept(std::declval<self_t &>().try_pop())) {
        while (true) {
            auto v = this->try_pop();
            if (!v) {
                break;
            }
        }
    }

    bool peek(T& out) const noexcept(std::is_nothrow_copy_assignable_v<T> || std::is_nothrow_copy_constructible_v<T>) {
        const auto tail_loaded = tail.load(std::memory_order_relaxed);
        if (tail_loaded == head.load(std::memory_order_acquire)) {
            return false;
        }
        const std::size_t idx = (tail_loaded & Mask);
        auto* slot = reinterpret_cast<const T*>(&storage[idx * sizeof(T)]);
        out = *slot;
        return true;
    }

    std::size_t pop_bulk(T* out, std::size_t max_n) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
        const auto tail_loaded = tail.load(std::memory_order_relaxed);
        const auto head_loaded = head.load(std::memory_order_acquire);
        const std::size_t available = head_loaded - tail_loaded;
        if (available == 0 || max_n == 0) {
            return 0;
        }
        std::size_t to_pop = (available < max_n) ? available : max_n;
        std::size_t idx = (tail_loaded & Mask);
        std::size_t first = ((Capacity - idx) < to_pop) ? (Capacity - idx) : to_pop;

        for (std::size_t i = 0; i < first; ++i) {
            auto* slot = reinterpret_cast<T*>(&storage[(idx + i) * sizeof(T)]);
            out[i] = std::move(*slot);
            std::destroy_at(slot);
        }
        std::size_t popped = first;
        if (popped < to_pop) {
            for (std::size_t i = 0; i < to_pop - first; ++i) {
                auto* slot = reinterpret_cast<T*>(&storage[i * sizeof(T)]);
                out[popped + i] = std::move(*slot);
                std::destroy_at(slot);
            }
        }
        tail.store(tail_loaded + to_pop, std::memory_order_release);
        return to_pop;
    }

    bool emplace_bulk(T&& elements...) {
        return false;
    }

private:
    alignas(alignof(T)) unsigned char storage[Capacity * sizeof(T)] {};

    alignas(64) std::atomic<std::size_t> head;
    alignas(64) std::atomic<std::size_t> tail;
};

}
