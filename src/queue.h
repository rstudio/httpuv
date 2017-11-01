#ifndef QUEUE_HPP
#define QUEUE_HPP

// A thread-safe queue, using threading constructs from libuv.

#include <queue>
#include "guard.h"

template <typename T>
class queue {

private:
  std::queue<T> q;

public:
  queue();

  void push(const T&);
  T& front();
  void pop();
  int size();

  uv_mutex_t mutex;
};


template <typename T>
queue<T>::queue() {
  uv_mutex_init_recursive(&mutex);
  q = std::queue<T>();
}

template <typename T>
void queue<T>::push(const T& item) {
  guard guard(mutex);
  q.push(item);
}

template <typename T>
T& queue<T>::front() {
  guard guard(mutex);
  T& item = q.front();
  return item;
}

template <typename T>
void queue<T>::pop() {
  guard guard(mutex);
  q.pop();
}

template <typename T>
int queue<T>::size() {
  guard guard(mutex);
  return q.size();
}


#endif // QUEUE_HPP
