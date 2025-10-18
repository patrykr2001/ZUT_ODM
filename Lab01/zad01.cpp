#include <iostream>
#include <thread>
#include <vector>

const int num_threads = 4;
const int N = 1000;
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
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(func, i));
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}