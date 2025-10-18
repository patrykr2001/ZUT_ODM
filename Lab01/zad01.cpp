#include <iostream>
#include <thread>
#include <vector>

int main() {
    const int num_threads = 5;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Hello from thread " << i << std::endl;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}