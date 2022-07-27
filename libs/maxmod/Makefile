src = $(wildcard *.S)
obj = $(src:.S=.o)
lib_a = libmm.a

TCPREFIX ?= arm-none-eabi-
CC = $(TCPREFIX)gcc
AS = $(TCPREFIX)as
AR = $(TCPREFIX)ar

ASFLAGS = -mcpu=arm7tdmi -mtune=arm7tdmi -mthumb -mthumb-interwork -DSYS_GBA -DUSE_IWRAM

$(lib_a): $(obj)
	$(AR) rcs $@ $(obj)

.PHONY: clean
clean:
	rm -f $(obj) $(lib_a)
