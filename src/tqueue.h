#ifndef TQUEUE_HPP
#define TQUEUE_HPP

// A thread-safe queue, using threading constructs from libuv.

#include <queue>
#include "thread.h"

template <typename T>
class tqueue {

private:
  std::queue<T> q;

public:
  tqueue();

  void push(const T&);
  T& front();
  void pop();
  int size();

  uv_mutex_t mutex;
};


template <typename T>
tqueue<T>::tqueue() {
  uv_mutex_init_recursive(&mutex);
  q = std::queue<T>();
}

template <typename T>
void tqueue<T>::push(const T& item) {
  guard guard(mutex);
  q.push(item);
}

template <typename T>
T& tqueue<T>::front() {
  guard guard(mutex);
  T& item = q.front();
  return item;
}

template <typename T>
void tqueue<T>::pop() {
  guard guard(mutex);
  q.pop();
}

template <typename T>
int tqueue<T>::size() {
  guard guard(mutex);
  return q.size();
}


#endif // TQUEUE_HPP
