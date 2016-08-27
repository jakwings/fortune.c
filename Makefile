PROGRAMS = fortune str unstr
BINS = $(PROGRAMS:%=bin/%)

CFLAGS += -std=c99 -Oz -Iinclude

PREFIX = /usr
FORTUNE_DATA_DIR := $(abspath $(PREFIX)/share/fortune)
CFLAGS += -DFORTUNE_DATA_DIR="\"$(FORTUNE_DATA_DIR)\""

all: $(BINS) cookies

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -DFORTUNE_MAKE_LIB -o $@ -c $<

lib/%.a: obj/%.o
	@mkdir -p lib
	$(AR) rcu $@ $<

bin/fortune: src/fortune.c lib/str.a lib/unstr.a
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^

bin/str: src/str.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

bin/unstr: src/unstr.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@rm -f test/cookies1.dat && \
		bin/fortune --index -s test/cookies1 | grep ': 3$$'
	@bin/fortune --dump -s test/cookies1 test/cookies1.txt && \
		diff -qs test/cookies1{,.txt}
	@rm -f test/cookies2.dat && \
		bin/fortune --index -s test/cookies2 test/cookies2.dat | grep ': 5$$'
	@bin/fortune --dump -s test/cookies2 test/cookies2.txt && \
		! diff -qs test/cookies2{,.txt}
	@rm -f test/cookies1.dat && \
		bin/fortune --index -cs test/cookies1 | grep ': 2$$'
	@bin/fortune --dump -s test/cookies1 test/cookies1.txt && \
		! diff -qs test/cookies1{,.txt}
	@rm -f test/cookies2.dat && \
		bin/fortune --index -cs test/cookies2 | grep ': 2$$'
	@bin/fortune --dump -s test/cookies2 test/cookies2.txt && \
		! diff -qs test/cookies2{,.txt}
	@rm -f test/cookies3.dat && \
		bin/fortune --index -xs test/cookies3 | grep ': 3$$'
	@bin/fortune --dump -xs test/cookies3 test/cookies3.txt && \
		diff -qs test/cookies1 test/cookies3.txt
	@rm -f test/cookies3.dat && \
		bin/fortune --index -cxs test/cookies3 | grep ': 2$$'
	@bin/fortune --dump -cxs test/cookies3 test/cookies3.txt && \
		! diff -qs test/cookies1 test/cookies3.txt
	@rm -f test/*.* && \
		for f in test/*; do ./bin/fortune --index -c $$f; done && \
		bin/fortune test

cookies: bin/str $(wildcard data/*)
	@mkdir -p cookies
	@rm -f cookies/*
	cp data/* cookies
	for f in cookies/*; do bin/str -cs $$f; done

install: bin/fortune cookies
	mkdir -p $(PREFIX)/{bin,share/fortune}
	cp bin/fortune $(PREFIX)/bin
	cp cookies/* $(PREFIX)/share/fortune

uninstall:
	rm -ri $(PREFIX)/{bin/fortune,share/fortune}

clean:
	rm -rf test/*.{dat,txt} bin obj lib cookies

.PHONY: all clean
