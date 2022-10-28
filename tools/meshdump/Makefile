src = $(wildcard *.c)
obj = $(src:.c=.o)
bin = meshdump

CFLAGS = -pedantic -Wall -g -DNO_OPENGL
LDFLAGS = -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
