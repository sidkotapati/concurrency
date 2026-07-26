#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <utility>

/// BRIEF: MPMC IMPL -- BASED ON VYUKOV IMPL, NO AI GENERATED CODE

template<typename T, size_t SIZE>
class MPMC_Queue {
    /// NOTE: When the queue is of size 1, the tail_ can't block the head with incrementing by sizeof(queue) 
    /// because that's indistinguishable from the head's increment (which is also 1). Hence the try_push()
    /// call thinks it's own increment was the tails consumption and tail_ can become larger than head_.
    static_assert(SIZE > 1, "MPMC_Queue SIZE must be greater than 1");
    
    /// NOTE: head_ and tail_ can be relaxed because they don't publish or consume the actual values 
    ///      therefore aren't responsible for synchronizing them across threads. seq_nums are responsible
    ///      for this, hence acq_rel semantics.
public:
    MPMC_Queue() {
        for (uint64_t i{ 0ull }; i < SIZE; ++i) {
            data_[i].seq_num.store(i, std::memory_order_relaxed);
        }
    }

    template <typename U>
    [[nodiscard]] bool try_push(U&& element) {
        uint64_t head_pos{ head_.load(std::memory_order_relaxed) }; 
        const uint64_t seq_num_at_head{ data_[head_pos % SIZE].seq_num.load(std::memory_order_acquire) };

        // Guards against enqueue between the loads and fullness
        if (head_pos != seq_num_at_head) return false; 
        const uint64_t next_head{ head_pos + 1 };
        const bool reserve_succeeded{ head_.compare_exchange_weak(head_pos, next_head,
                                                                    std::memory_order_relaxed,
                                                                    std::memory_order_relaxed) };
        if (!reserve_succeeded) return false;

        data_[head_pos % SIZE].value = std::forward<U>(element);

        data_[head_pos % SIZE].seq_num.fetch_add(1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> try_pop() {
        uint64_t tail_pos{ tail_.load(std::memory_order_relaxed) };
        const uint64_t seq_num_at_tail{ data_[tail_pos % SIZE].seq_num.load(std::memory_order_acquire) };

        // Guard against dequeue between loads and emptiness
        if (seq_num_at_tail == tail_pos) return std::nullopt; 
        const uint64_t next_tail{ tail_pos + 1 };
        const bool reserve_succeeded{ tail_.compare_exchange_weak(tail_pos, next_tail,
                                                                std::memory_order_relaxed,
                                                                std::memory_order_relaxed) };
        if (!reserve_succeeded) return std::nullopt;
        
        T popped_value = std::move_if_noexcept(data_[tail_pos % SIZE].value);

        data_[tail_pos % SIZE].seq_num.fetch_add(SIZE - 1, std::memory_order_release);

        return popped_value;
    }


private:
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> head_{ 0ull };
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> tail_{ 0ull };
    struct Slot {
        T value;
        std::atomic<uint64_t> seq_num;
    };
    alignas(std::hardware_destructive_interference_size) std::array<Slot, SIZE> data_;
};
