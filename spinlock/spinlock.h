#pragma once

#include <atomic>
#include <new>

/// BRIEF: SPINLOCK IMPL, NO AI GENERATED CODE

class SpinLock {
private:
    enum class lock_stat : bool {
        LOCKED, UNLOCKED
    };

public:
    void lock() {
        lock_stat EXPECTED{lock_stat::UNLOCKED};
        constexpr lock_stat DESIRED{lock_stat::LOCKED};
        while (flag_.load(std::memory_order_acquire) != EXPECTED ||
                flag_.exchange(DESIRED, std::memory_order_acq_rel) != EXPECTED)
        // while (!flag_.compare_exchange_weak(EXPECTED,
        //                                     DESIRED, 
        //                                     std::memory_order_acquire,
        //                                     std::memory_order_relaxed)) 
        {
            // EXPECTED = lock_stat::UNLOCKED;
        }
    }
    
    void unlock() {
        flag_.store(lock_stat::UNLOCKED, std::memory_order_release);
    }

private:
    alignas(std::hardware_destructive_interference_size) std::atomic<lock_stat> flag_{lock_stat::UNLOCKED}; 
};
