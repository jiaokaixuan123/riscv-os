# Minimal Makefile to build and run on qemu-system-riscv64
CROSS  ?= riscv64-unknown-elf-
CC     := $(CROSS)gcc
LD     := $(CROSS)ld
OBJCOPY:= $(CROSS)objcopy

CFLAGS := -Wall -Wextra -Os -ffreestanding -nostdlib -nostartfiles -march=rv64gc -mabi=lp64 -I. -mcmodel=medany -fno-pic -DDEBUG
LDFLAGS:= -T kernel.ld -nostdlib -z max-page-size=4096

# Auto-discover sources in the current directory
CSRCS := $(wildcard *.c)
ASRCS_UP := $(wildcard *.S)
ASRCS_LO := $(wildcard *.s)
SRCS := $(CSRCS) $(ASRCS_UP) $(ASRCS_LO)

OBJS := $(CSRCS:.c=.o)
OBJS += $(ASRCS_UP:.S=.o)
OBJS += $(ASRCS_LO:.s=.o)

all: kernel.elf

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

# === 链接规则（缺这个就会报 No rule to make target 'kernel.elf'）===
kernel.elf: $(OBJS) kernel.ld
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# ---- QEMU config (短且好记) ----
QEMU      := qemu-system-riscv64
QEMU_ARGS := -M virt -bios none -kernel kernel.elf -serial mon:stdio -display none


run: kernel.elf
	@$(QEMU) $(QEMU_ARGS)

runraw: kernel.elf
	@$(QEMU) $(QEMU_ARGS)

# 只运行并保留退出码，便于脚本化
runquiet: kernel.elf
	$(QEMU) $(QEMU_ARGS) >/dev/null 2>&1 || true

clean:
	@rm -f $(OBJS) kernel.elf

.PHONY: all run runraw clean
