all: webserver

webserver: webserver.c libuv/libuv.a http-parser/http_parser.o
	g++ -Wall -I"$(SRCDIR)libuv/include" -I"$(SRCDIR)http-parser" \
		-I"/home/jcheng/R/x86_64-pc-linux-gnu-library/2.15/BH/include" \
		-o webserver webserver.c "$(SRCDIR)libuv/libuv.a" \
		"$(SRCDIR)http-parser/http_parser.o" -lrt

libuv/libuv.a:
	$(MAKE) --directory "$(SRCDIR)libuv"

http-parser/http_parser.o:
	$(MAKE) --directory "$(SRCDIR)http-parser" http_parser.o
