# PixelForge3DS - 3DS homebrew build using devkitARM + libctru
# Requires devkitPro's 3ds-dev package and 3dsxtool.
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set. Install devkitPro 3ds-dev or use GitHub Actions)
endif
ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set)
endif

TARGET := PixelForge3DS
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
CFLAGS := -O2 -g -Wall -Wextra -std=c11 -ffunction-sections -fdata-sections \
          -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
          -I$(DEVKITPRO)/libctru/include -D__3DS__
LDFLAGS := -specs=3dsx.specs -Wl,--gc-sections -L$(DEVKITPRO)/libctru/lib
LDLIBS := -lctru -lm

.PHONY: all clean
all: $(TARGET).3dsx

$(TARGET).elf: source/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

clean:
	rm -f $(TARGET).elf $(TARGET).3dsx
