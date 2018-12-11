#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

template <typename T>
class threadsafe_queue {
private:
    mutable mutex mut;
    queue<T> data_queue;
    condition_variable data_cond;

public:
    threadsafe_queue()
    {}

    void push(T new_value) {
        lock_guard<mutex> lk(mut);
        data_queue.push(move(new_value));
        data_cond.notify_one();
    }

    bool try_pop(T& value) {
        lock_guard<mutex> lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        value = move(data_queue.front());
        data_queue.pop();
        return true;
    }

    bool empty() const {
        lock_guard<mutex> lk(mut);
        return data_queue.empty();
    }
};

#endif

