#include <iostream>
#include <thread>
#include <vector>

int num_threads = 1;
const int N = 4096;
int A[N][N], B[N][N], C[N][N];

void func(int tid) {
    int i, j, k;
    int lb = (tid * N) / num_threads;
    int ub = ((tid + 1) * N) / num_threads;

    for(i = lb; i < ub; i++) {
        for(j = 0; j < N; j++) {
            C[i][j] = 0;
            for(k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main() {
    for(int v = 1; v <= 16; v *= 2) {
        num_threads = v;
        std::vector<std::thread> threads;
        const auto start{std::chrono::steady_clock::now()};

        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(func, i));
        }

        for (auto& t : threads) {
            t.join();
        }

        const auto finish{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{finish - start};
        std::cout << "Threads: " << num_threads << ", Elapsed time: " << elapsed_seconds.count() << "s\n";
    }

    return 0;
}