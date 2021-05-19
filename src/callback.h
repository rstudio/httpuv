#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <functional>
#include <later_api.h>

class Callback {
public:
  virtual ~Callback() {};
  virtual void operator()() = 0;
};

// If the Callback class were integrated into later, this wouldn't be
// necessary -- later could accept a void(Callback*) function.
void invoke_callback(void* data);


// Wrapper class for std functions
class StdFunctionCallback : public Callback {
private:
  std::function<void (void)> fun;

public:
  StdFunctionCallback(std::function<void (void)> fun)
    : fun(fun) {
  }

  void operator()() {
    fun();
  }

};

void invoke_later(std::function<void(void)> f, double secs = 0);

#endif
