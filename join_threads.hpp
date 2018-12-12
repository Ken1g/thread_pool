#ifndef JOIN_THREADS
#define JOIN_THREADS

#include <thread>
#include <iostream>

using namespace std;

class join_threads {
private:
    vector<thread>& threads;

public:
    explicit join_threads(vector<thread>& threads_) :
        threads(threads_)
    {}

    ~join_threads() {
        for (unsigned long i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
};

#endif



