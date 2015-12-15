DEP_CC:=gcc  -I. -nostdinc -ggdb3 -O0 -std=c99 -Wall -Werror -std=gnu99 -m32 -ffunction-sections -ffreestanding -fno-omit-frame-pointer -Wall -W -Wshadow -Wno-format -Werror -gdwarf-2 -fno-stack-protector -MD -MF .deps/.d -MP  _  -Os --gc-sections -m elf_i386
DEP_PREFER_GCC:=
