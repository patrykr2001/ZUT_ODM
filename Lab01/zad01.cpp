#include <iostream>
#include <thread>
#include <vector>

int num_threads = 1;
const int N = 4096;
int A[N][N], B[N][N], C[N][N], BT[N][N];

// Dynamic arrays for funcDynamic
int** A_dyn = nullptr;
int** B_dyn = nullptr;
int** C_dyn = nullptr;
int** BT_dyn = nullptr;

void funcStatic(int tid, bool transposed = false) {
    int i, j, k;
    int lb = (tid * N) / num_threads;
    int ub = ((tid + 1) * N) / num_threads;

    for(i = lb; i < ub; i++) {
        for(j = 0; j < N; j++) {
            C[i][j] = 0;
            for(k = 0; k < N; k++) {
                if (transposed) {
                    C[i][j] += A[i][k] * BT[j][k];
                } else {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }
}

void funcDynamic(int tid, bool transposed = false) {
    int i, j, k;
    int lb = (tid * N) / num_threads;
    int ub = ((tid + 1) * N) / num_threads;

    for(i = lb; i < ub; i++) {
        for(j = 0; j < N; j++) {
            C_dyn[i][j] = 0;
            for(k = 0; k < N; k++) {
                if (transposed) {
                    C_dyn[i][j] += A_dyn[i][k] * BT_dyn[j][k];
                } else {
                    C_dyn[i][j] += A_dyn[i][k] * B_dyn[k][j];
                }
            }
        }
    }
}

void transpose() {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            BT[j][i] = B[i][j];
        }
    }
}

void transposeDynamic() {
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            BT_dyn[j][i] = B_dyn[i][j];
        }
    }
}

void allocateDynamicArrays() {
    A_dyn = new int*[N];
    B_dyn = new int*[N];
    C_dyn = new int*[N];
    BT_dyn = new int*[N];
    
    for(int i = 0; i < N; i++) {
        A_dyn[i] = new int[N];
        B_dyn[i] = new int[N];
        C_dyn[i] = new int[N];
        BT_dyn[i] = new int[N];
    }
}

void freeDynamicArrays() {
    for(int i = 0; i < N; i++) {
        delete[] A_dyn[i];
        delete[] B_dyn[i];
        delete[] C_dyn[i];
        delete[] BT_dyn[i];
    }
    
    delete[] A_dyn;
    delete[] B_dyn;
    delete[] C_dyn;
    delete[] BT_dyn;
}

int main() {
    // Allocate dynamic arrays
    allocateDynamicArrays();
    
    std::cout << "Static Arrays:\n";
    for(int v = 1; v <= 16; v *= 2) {
        num_threads = v;
        std::vector<std::thread> threads;
        const auto start{std::chrono::steady_clock::now()};

        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(funcStatic, i, false));
        }

        for (auto& t : threads) {
            t.join();
        }

        const auto finish{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{finish - start};
        std::cout << "Threads: " << num_threads << ", Elapsed time: " << elapsed_seconds.count() << "s\n";
    }

    transpose();
    
    std::cout << "Static Arrays Transpose:\n";
    for(int v = 1; v <= 16; v *= 2) {
        num_threads = v;
        std::vector<std::thread> threads;
        const auto start{std::chrono::steady_clock::now()};

        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(funcStatic, i, true));
        }

        for (auto& t : threads) {
            t.join();
        }

        const auto finish{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{finish - start};
        std::cout << "Threads: " << num_threads << ", Elapsed time: " << elapsed_seconds.count() << "s\n";
    }

    std::cout << "Dynamic Arrays:\n";
    for(int v = 1; v <= 16; v *= 2) {
        num_threads = v;
        std::vector<std::thread> threads;
        const auto start{std::chrono::steady_clock::now()};

        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(funcDynamic, i, false));
        }

        for (auto& t : threads) {
            t.join();
        }

        const auto finish{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{finish - start};
        std::cout << "Threads: " << num_threads << ", Elapsed time: " << elapsed_seconds.count() << "s\n";
    }

    transposeDynamic();

    std::cout << "Dynamic Arrays Transpose:\n";
    for(int v = 1; v <= 16; v *= 2) {
        num_threads = v;
        std::vector<std::thread> threads;
        const auto start{std::chrono::steady_clock::now()};

        for (int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread(funcDynamic, i, true));
        }

        for (auto& t : threads) {
            t.join();
        }

        const auto finish{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{finish - start};
        std::cout << "Threads: " << num_threads << ", Elapsed time: " << elapsed_seconds.count() << "s\n";
    }

    // Free dynamic arrays
    freeDynamicArrays();

    return 0;
}