#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <optional>
#include <iostream>

#include "spsc_queue.h"

TEST_CASE("spsc queue test") {
    // Queue of size 1 should produce the most EMPTY / FULL behavior
    SPSC_Queue<int, 1> fifo_queue;

    std::thread producer([&](){
        for(int i{0}; i < 10000; ++i) {
            while (!fifo_queue.try_push(i)) {}
        }
    });

    std::thread consumer([&](){
        for(int i{0}; i < 10000; ++i) {
            std::optional<int> popped_value{ std::nullopt };
            while (!(popped_value = fifo_queue.try_pop())) {}
            CHECK(*popped_value == i);
        }
    });

    // no jthread for some reason :(
    // i think Apple libc++ isn't supporting right now ?
    producer.join();
    consumer.join(); 
}
