src = $(wildcard src/*.c)
ssrc = $(wildcard src/*.s) data/lut.s
obj = $(src:.c=.o) $(ssrc:.s=.o)
dep = $(src:.c=.d)
name = gbajam22
elf = $(name).elf
bin = $(name).gba

libs = libs/maxmod/libmm.a

TCPREFIX = arm-none-eabi-

CPP = $(TCPREFIX)cpp
CC = $(TCPREFIX)gcc
AS = $(TCPREFIX)as
OBJCOPY = $(TCPREFIX)objcopy
OBJDUMP = $(TCPREFIX)objdump

opt = -O3 -fomit-frame-pointer -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork
dbg = -g
inc = -I. -Ilibs/maxmod

CFLAGS = $(opt) $(dbg) -pedantic -Wall -MMD $(def) $(inc)
ASFLAGS = -mthumb-interwork
LDFLAGS = -mthumb -mthumb-interwork $(libs) -lm

-include cfg.mk

.PHONY: all
all: $(bin) $(bin_mb)

$(bin): $(elf)
	$(OBJCOPY) -O binary $(elf) $(bin)
	gbafix -r0 $(bin)

$(elf): $(obj) $(libs)
	$(CC) -o $(elf) $(obj) -specs=gba.specs -Wl,-Map,link.map $(LDFLAGS)

-include $(dep)

src/data.o: src/data.s $(data)

tools/pngdump/pngdump:
	$(MAKE) -C tools/pngdump

tools/lutgen: tools/lutgen.c
	cc -o $@ $< -lm

tools/mmutil/mmutil:
	$(MAKE) -C tools/mmutil

%.sraw: %.png tools/pngdump/pngdump
	tools/pngdump/pngdump -o $@ -oc $(subst .sraw,.spal,$@) -os $(subst .sraw,.shade,$@) -s 8 $<

%.raw: %.png tools/pngdump/pngdump
	tools/pngdump/pngdump -o $@ -n $<

%.pal: %.png tools/pngdump/pngdump
	tools/pngdump/pngdump -o $@ -c $<

data/lut.s: tools/lutgen
	tools/lutgen >$@

data/snd.bin: $(audata) tools/mmutil/mmutil
	tools/mmutil/mmutil -o$@ -hdata/snd.h $(audata)

data/snd.h: data/snd.bin

.PHONY: clean
clean:
	rm -f $(obj) $(bin) $(bin_mb) $(elf) $(elf_mb)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: cleanlibs
cleanlibs:
	$(MAKE) -C libs/maxmod clean

.PHONY: install
install: $(bin)
	if2a -n -f -W $<

.PHONY: run
run: $(bin)
	mgba -2 $(bin)

.PHONY: debug
debug: $(elf)
	mgba -2 -g $(bin) &
	$(TCPREFIX)gdb $<

.PHONY: disasm
disasm: $(elf)
	$(OBJDUMP) -d $< >$@

.PHONY: libs
libs: $(libs)

libs/maxmod/libmm.a:
	$(MAKE) -C libs/maxmod
