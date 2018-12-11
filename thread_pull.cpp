#define A_SIZE 10

#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <limits.h>
#include <condition_variable>
#include "thread_safe_queue.hpp"
#include "join_threads.hpp"

using namespace std;

struct StateData {
    mutable mutex m;
    condition_variable c;

    void Wait() {
        unique_lock<mutex> lock(m);
        c.wait(lock);
    }

    void Finish() {
        c.notify_one();
    }
};

typedef shared_ptr<StateData> State;

struct sort_function {
    function<void(int*, int, int)> task;
    int* a;
    int st;
    int fn;
    State state;
};

class thread_pool {
private:
    atomic_bool done;
    threadsafe_queue<sort_function> work_queue;
    vector<thread> threads;
    join_threads joiner;

    void worker_thread() {
        while (!done) {
            sort_function sf;
            if (work_queue.try_pop(sf)) {
                sf.task(sf.a, sf.st, sf.fn);
            }
        }
    }

public:
    thread_pool():
        done(false), joiner(threads) {
        unsigned const thread_count = thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < thread_count; ++i) {
                threads.push_back(
                    thread(&thread_pool::worker_thread, this));
            }
        }
        catch(...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() {
        done = true;
    }

    void submit(sort_function f) {
        work_queue.push(f);
    }
};

void merge(int* A, int st, int mid, int fn) {
    int n_l = mid - st + 1;
    int n_r = fn - mid;
    int* L = new int[A_SIZE];
    int* R = new int[A_SIZE];
    for (int i = 0; i < n_l; ++i) {
        L[i] = A[st + i];
    }
    for (int i = 0; i < n_r; ++i) {
        R[i] = A[mid + i + 1];
    }
    L[n_l] = INT_MAX;
    R[n_r] = INT_MAX;
    int j = 0;
    int k = 0;
    for (int i = st; i <= fn; ++i) {
        if (L[j] < R[k]) {
            A[i] = L[j];
            j++;
        }
        else {
            A[i] = R[k];
            k++;
        }
    }
    delete[] L;
    delete[] R;
}

void merge_sort(int* a, int st, int fn) {
    if (st != fn) {
        int mid = (st + fn) / 2;
        merge_sort(a, st, mid);
        merge_sort(a, mid + 1, fn);
        merge(a, st, mid, fn);
    }
}

int main() {
    int* A = new int[A_SIZE];
    for (int i = 0; i < A_SIZE; ++i) {
        A[i] = 100 - i;
    }

    thread_pool tp;
    sort_function sf {merge_sort, A, 0, A_SIZE - 1};
    tp.submit(sf);
    sf.state->Wait();


//    std::this_thread::sleep_for (std::chrono::seconds(1));
//    for (int i = 0; i < A_SIZE; ++i) {
//        cout << A[i] << " ";
//    }
//    cout << endl;
    delete[] A;

    return 0;
}
