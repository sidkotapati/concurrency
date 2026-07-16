#include <atomic>
#include <array>
#include <optional>
#include <new>

/// BRIEF: SPSC IMPL -- NO AI GENERATED CODE

///  NOTE: Adding an extra spot in the buffer to differentiate between full and empty.
///        Relieves the need for a size_ variable (and synchronization for that memory).
///        head_ is one past the front slot and push writes to that position.
///        tail_ is exactly the back slot and pop reads from that position.
/// 
///  Keeping track of EMPTY / FULL states as follows:
///                      head_ == tail_      ===> EMPTY
///  (head_ + 1) % BUFFER_SIZE == tail_      ===> FULL

template <typename T, size_t SIZE>
class SPSC_Queue {
private:
    static constexpr size_t BUFFER_SIZE{ SIZE + 1 };
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> head_{ 0ull };
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> tail_{ 0ull };

    alignas(std::hardware_destructive_interference_size) std::array<T, BUFFER_SIZE> data_;


public:
    template<typename U>
    bool try_push(U&& element) {
        const uint64_t head_pos{ head_.load(std::memory_order_relaxed) };
        const uint64_t new_head_pos{ (head_pos + 1) % BUFFER_SIZE };
        if (new_head_pos == tail_.load(std::memory_order_acquire)) return false; // FULL

        data_[head_pos] = std::forward<U>(element);
        head_.store(new_head_pos, std::memory_order_release);
        return true; 
    }


    std::optional<T> try_pop() {
        const uint64_t tail_pos{ (tail_.load(std::memory_order_relaxed)) % BUFFER_SIZE };
        if (tail_pos == head_.load(std::memory_order_acquire)) return std::nullopt;
        
        const T to_be_returned{ std::move_if_noexcept(data_[tail_pos]) };

        const uint64_t new_tail_pos{ (tail_pos + 1) % BUFFER_SIZE };
        tail_.store(new_tail_pos, std::memory_order_release);
        return to_be_returned;
    }

};