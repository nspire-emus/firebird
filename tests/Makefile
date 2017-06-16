SRCS := $(shell find . -name \*.S)
OBJS := $(patsubst %.S, %.bin, $(SRCS))

test: run_tests.sh $(OBJS)
	./run_tests.sh

.PHONY: test

%.bin: %.o
	arm-none-eabi-objcopy -O binary $^ $@

%.o: %.S
	arm-none-eabi-as -c $^ -o $@
