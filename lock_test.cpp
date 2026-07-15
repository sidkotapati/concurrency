#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include "seqlock/seqlock.h"
#include "spinlock/spinlock.h"

TEST_CASE("spinlock serializes increments") {
    SpinLock lock;
    int value = 0;
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 1000; ++j) {
                lock.lock();
                ++value;
                lock.unlock();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    REQUIRE(value == 4000);
}

TEST_CASE("seqlock reads latest complete write") {
    Seqlock<int> lock(1);

    REQUIRE(lock.read() == 1);
    lock.write(2);
    REQUIRE(lock.read() == 2);
}

TEST_CASE("seqlock supports concurrent reads and writes") {
    Seqlock<int> lock(0);
    std::atomic<bool> done{false};
    std::thread writer([&] {
        for (int i = 1; i <= 1000; ++i) {
            lock.write(int{i});
        }
        done = true;
    });

    bool ok = true;
    while (!done) {
        auto value = lock.read();
        ok = ok && (!value || *value >= 0);
    }

    writer.join();
    REQUIRE(ok);
    REQUIRE(lock.read() == 1000);
}
