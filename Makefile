CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=c11
BUILD   := build
SRCS    := $(shell find 0[1-9]_* 1[0-5]_* -name '*.c' ! -path '*/ui/*' ! -path '*/tests/*')
OBJS    := $(SRCS:%.c=$(BUILD)/%.o) $(BUILD)/registry.o
BIN     := $(BUILD)/talco-os3
INC     := -I02_TALCO_CORE/system_api/include

.PHONY: all run shell test image clean os os-arm64 qemu ui
all: $(BIN)

$(BUILD)/registry.c: $(SRCS) 17_BUILD_SYSTEM/toolchains/gen_registry.py
	@python3 17_BUILD_SYSTEM/toolchains/gen_registry.py . $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BUILD)/registry.o: $(BUILD)/registry.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@

run: $(BIN)
	./$(BIN)
shell: $(BIN)
	./$(BIN) --shell
test: $(BIN)
	./18_QUALITY_ASSURANCE/integration_tests/run_tests.sh
	$(MAKE) -C talco-kernel test
image: $(BIN)
	./17_BUILD_SYSTEM/image_builder/make_image.sh
clean:
	rm -rf $(BUILD)
	$(MAKE) -C talco-kernel clean

os:
	$(MAKE) -C talco-kernel
qemu:
	$(MAKE) -C talco-kernel qemu

os-arm64:
	$(MAKE) -C talco-kernel ARCH=aarch64
ui:
	@mkdir -p build
	gcc -Wall -Wextra -o build/render 05_TALCO_EXPERIENCE/launcher/tests/render.c 05_TALCO_EXPERIENCE/launcher/ui/gfx.c 05_TALCO_EXPERIENCE/launcher/ui/launcher.c && cd build && ./render
