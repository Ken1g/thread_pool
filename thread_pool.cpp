#define A_SIZE 12345678

#include <iostream>
#include <ctime>
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
    atomic_bool task_done;

    StateData() {
        task_done = false;
    }

    void Wait() {
        if (!task_done) {
            unique_lock<mutex> lock(m);
            c.wait(lock);
        }
    }

    void Finish() {
        task_done = true;
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
                sf.state->Finish();
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
    int* L = new int[n_l + 1];
    int* R = new int[n_r + 1];
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

void global_merge(int* A, int n) {
    for (int i = 0; i < n - 1; ++i) {
        int l = (i + 1) * (A_SIZE / n) - 1;
        int r = (i == n - 2) ? A_SIZE - 1 : l + (A_SIZE / n);
        merge(A, 0, l, r);
    }
}

void print_A(int* A) {
    for (int i = 0; i < A_SIZE; ++i) {
        cout << A[i] << " ";
    }
    cout << endl;
}

bool check(int* A) {
    bool ans = true;
    for (int i = 1; i < A_SIZE; ++i) {
        if (A[i] < A[i - 1]) {
            ans = false;
            break;
        }
    }
    return ans;
}

int main() {
    int max_number_of_threads = 6;

    cout <<  "A_SIZE = " << A_SIZE << endl;
    for (unsigned int i = 1; i <= max_number_of_threads; ++i)
    {
        int* A = new int[A_SIZE];
        for (int i = 0; i < A_SIZE; ++i) {
            A[i] = 100 - i;
        }

        thread_pool tp;
        vector<State> states;
        int interval = A_SIZE / i;

        clock_t start;
        start = clock();
        for (int j = 0; j < i; ++j) {
            int st = j * interval;
            int fn = (j == (i - 1)) ? A_SIZE - 1 : (st - 1 + interval);
            State state{new StateData};
            states.push_back(state);
            sort_function sf {merge_sort, A, st, fn, state};
            tp.submit(sf);
        }
        for (int j = 0; j < i; ++j) {
            states[j]->Wait();
        }

        global_merge(A, i);
        cout << (check(A) ? "CORRECT" : "NOT CORRECT") << endl;
        cout << "Number of threads: " << i << endl;
        cout << "Time: " << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000) 
                         << " ms" << std::endl;
        cout << endl;

        delete[] A;
    }

    return 0;
}
