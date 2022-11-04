CFLAGS = -pedantic -Wall -Wno-unused-function -g
LDFLAGS = -lpng -lz -lm

pngdump: main.o image.o quant.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) main.o
	$(RM) image.o
	$(RM) pngdump
