#ifndef CONSTANTS_H
#define CONSTANTS_H

enum Opcode {
  Continuation = 0,
  Text = 1,
  Binary = 2,
  Close = 8,
  Ping = 9,
  Pong = 0xA,
  Reserved = 0xF
};

const size_t MAX_HEADER_BYTES = 14;

enum WSParseState {
  InHeader,
  InPayload
};

#endif // CONSTANTS_H
