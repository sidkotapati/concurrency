#include <atomic>
#include <utility>
#include <mutex>
#include <concepts>

#include "spinlock/spinlock.h"

/// BRIEF: SEQLOCK IMPL, NO AI GENERATED CODE

/// Basically a type of reader-writer lock where it's assumed writes happen infrequently.
/// Since writes are infrequent, they take priority over concurrent reads.

template <typename T> 
    requires std::atomic<T>::is_always_lock_free
class Seqlock {
private:
    SpinLock spinlock_;
    mutable std::atomic<uint64_t> seq_num_{ 0ull };
    std::atomic<T> value_;

    static constexpr uint64_t ODD_MASK{ 1ull };
    [[nodiscard]] bool is_odd(const uint64_t num) const noexcept { return (num & ODD_MASK) == 1; }

public:
    template<typename... Args>
    Seqlock(Args&&... args) : value_{std::forward<Args>(args)...} {}

    void write(T to_write) {
        std::lock_guard<SpinLock> guard(spinlock_); // Blocks other writers.
        seq_num_.fetch_add(1, std::memory_order_acq_rel); // Blocks other readers.
        value_.store(to_write, std::memory_order_relaxed);
        seq_num_.fetch_add(1, std::memory_order_acq_rel);
    }

    [[nodiscard]] std::optional<T> read() const {
        uint64_t initial_value{ 1ull }; // Odd forces compare_exchange retry.
        while(is_odd(initial_value)) {
            initial_value = seq_num_.load(std::memory_order_acquire);
        }

        T copy = value_.load(std::memory_order_relaxed);

        // Notice that it has to be the same as initial value (not just even)
        // This prevents bugs from a sneaky write that may have happened in between.
        if (seq_num_.load(std::memory_order_acquire) == initial_value) {
            return copy;
        }
        return std::nullopt;
    }
};
