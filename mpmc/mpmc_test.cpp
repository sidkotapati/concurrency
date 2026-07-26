#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <optional>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <syncstream>

#include "mpmc_queue.h"

TEST_CASE("mpmc queue test") {
    constexpr int N{ 10000 };
    std::vector<std::atomic<int>> answer_vec(N);
    for (auto& answer : answer_vec) {
        answer.store(0, std::memory_order_relaxed);
    }
    std::atomic<int> counter{ 0 };
    std::atomic<int> consumer_shutdown_count{ 0 };
    MPMC_Queue<int, 5> fifo_queue;

    auto producer_func = [&](){
        while (true) {
            const int curr_count{ counter.fetch_add(1, std::memory_order_relaxed) };
            if (curr_count >= N) break;
            while (!fifo_queue.try_push(curr_count)) {}
        }
    };

    auto consumer_func = [&](){
        while (consumer_shutdown_count.load(std::memory_order_relaxed) < N) {
            std::optional<int> popped_value{ fifo_queue.try_pop() };
            if (!popped_value.has_value()) continue;

            REQUIRE(*popped_value >= 0);
            REQUIRE(*popped_value < N);
            answer_vec[*popped_value].fetch_add(1, std::memory_order_relaxed);

            consumer_shutdown_count.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    std::vector<std::thread> producers_and_consumers;
    for (int i{0}; i < 1; ++i) {
        producers_and_consumers.emplace_back(producer_func);
        producers_and_consumers.emplace_back(consumer_func);
    }

    // no jthread for some reason :(
    // i think Apple libc++ isn't supporting right now ?
    while (!producers_and_consumers.empty()) {
        producers_and_consumers.back().join();
        producers_and_consumers.pop_back();
    }

    for (int i{0}; i < N; ++i) {
        const int num_times_added{ answer_vec[i].load(std::memory_order_relaxed) == 1 };
        // std::cerr << num_times_added << "\n";
        CHECK(num_times_added == 1);
    }
}
