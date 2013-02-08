OS := $(shell uname)

ifeq (Darwin,$(OS))
CPPFLAGS += -D_DARWIN_USE_64_BIT_INODE=1
LDFLAGS += -framework CoreServices
SOEXT = dylib
endif
ifeq (Linux,$(OS))
LDFLAGS += -lrt
SOEXT = so
endif

all: webserver

webserver: webserver.hpp webserver.cpp libuv/libuv.a http-parser/http_parser.o
	$(CXX) $(CPPFLAGS) -Wall -I"$(SRCDIR)libuv/include" -I"$(SRCDIR)http-parser" \
            -o webserver webserver.cpp "$(SRCDIR)libuv/libuv.a" \
                "$(SRCDIR)http-parser/http_parser.o" $(LDFLAGS)

libuv/libuv.a:
	$(MAKE) --directory "$(SRCDIR)libuv"

http-parser/http_parser.o:
	$(MAKE) --directory "$(SRCDIR)http-parser" http_parser.o
