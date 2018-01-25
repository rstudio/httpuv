#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <boost/function.hpp>
#include <later_api.h>

class Callback {
public:
  virtual ~Callback() {};
  virtual void operator()() = 0;
};

// If the Callback class were integrated into later, this wouldn't be
// necessary -- later could accept a void(Callback*) function.
void invoke_callback(void* data);


// Wrapper class for boost functions
class BoostFunctionCallback : public Callback {
private:
  boost::function<void (void)> fun;

public:
  BoostFunctionCallback(boost::function<void (void)> fun)
    : fun(fun) {
  }

  void operator()() {
    fun();
  }

};

void invoke_later(boost::function<void(void)> f, double secs = 0);

#endif
