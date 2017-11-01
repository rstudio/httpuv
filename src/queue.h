#ifndef QUEUE_HPP
#define QUEUE_HPP

// A thread-safe queue, using threading constructs from libuv.

#include <queue>

template <typename T>
class queue {

private:
    std::queue<T> q;
    uv_mutex_t mutex;

public:
    queue();

    void push(const T&);
    T& front();
    void pop();
    int size();

    void lock();
    void unlock();
};


template <typename T>
queue<T>::queue() {
    q = std::queue<T>();
    uv_mutex_init_recursive(&mutex);
}

template <typename T>
void queue<T>::push(const T& item) {
    lock();
    q.push(item);
    unlock();
}

template <typename T>
T& queue<T>::front() {
    lock();
    T& item = q.front();
    unlock();
    return item;
}

template <typename T>
void queue<T>::pop() {
    lock();
    q.pop();
    unlock();
}

template <typename T>
int queue<T>::size() {
    // TODO: Need guard instead
    // lock();
    return q.size();
    // unlock();
}

template <typename T>
void queue<T>::lock() {
    uv_mutex_lock(&mutex);
}

template <typename T>
void queue<T>::unlock() {
    uv_mutex_unlock(&mutex);
}

#endif // QUEUE_HPP
