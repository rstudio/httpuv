#ifndef DEFLATE_H
#define DEFLATE_H

#include <zlib.h>
#include <vector>

namespace deflator {

enum DeflateMode {
  DeflateModeZlib,
  DeflateModeRaw,
  DeflateModeGzip,
};

enum DeflatorState {
  DeflatorStateStart,
  DeflatorStateReady,
  DeflatorStateEnded
};

class Deflator {
private:
  z_stream _stream;
  DeflatorState _state;

public:
  Deflator();
  ~Deflator();
  int init(DeflateMode mode, int level = Z_DEFAULT_COMPRESSION,
    int windowBits = 13, int memLevel = 8,
    int strategy = Z_DEFAULT_STRATEGY);
  int reset();
  int deflate(const char* data, size_t data_len, std::vector<char>& output);
};

class Inflator {
private:
  z_stream _stream;
  DeflatorState _state;

public:
  Inflator();
  ~Inflator();
  int init(DeflateMode mode, int windowBits = 15);
  int reset();
  int inflate(const char* data, size_t data_len, std::vector<char>& output);
};

}

#endif // DEFLATE_H
