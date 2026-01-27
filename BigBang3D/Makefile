include $(DEVKITPRO)/libctru/base_rules

TARGET := BigBang3D
BUILD  := build
SOURCES := source
DATA := data
INCLUDES := include

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS := -g -Wall -O2 $(ARCH)

LIBS := -lctru -lcitro2d -lcitro3d -lm

all: $(TARGET).cia

$(TARGET).cia: $(TARGET).elf
	@makerom -f cia -o $@ -elf $< \
		-icon icon.bin \
		-banner banner.bin

$(TARGET).elf:
	$(MAKE) -C $(BUILD)

clean:
	rm -rf $(BUILD) *.elf *.cia
