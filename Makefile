CC = gcc
AS = as
LD = ld
CFLAGS = -m32 -std=gnu11 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
         -fno-builtin -fno-tree-loop-distribute-patterns -nostdlib -mno-sse -mno-mmx -O2 -Wall -Wextra

all: mosaicos.bin

boot.o: boot.s
	$(AS) --32 boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

mosaicos.bin: boot.o kernel.o linker.ld
	$(LD) -m elf_i386 -nostdlib -T linker.ld boot.o kernel.o -o mosaicos.bin

check: mosaicos.bin
	python3 tests/validate_image.py mosaicos.bin

check-desktop: check-050

check-050:
	python3 tests/validate_050.py

.PHONY: check-050

docker-iso:
	sh ./desktop/docker/build-with-docker.sh

.PHONY: docker-iso


run: mosaicos.bin
	qemu-system-i386 -kernel mosaicos.bin -m 256 -netdev user,id=n0 -device rtl8139,netdev=n0 -device piix3-usb-uhci -device usb-tablet

iso: mosaicos.bin
	mkdir -p iso/boot/grub && cp mosaicos.bin iso/boot/
	printf 'set timeout=0\nmenuentry "MosaicOS" { multiboot /boot/mosaicos.bin }\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o MosaicOS.iso iso

clean:
	rm -rf *.o mosaicos.bin iso MosaicOS.iso

.PHONY: all run check check-desktop iso clean

check-compatibility:
	python3 -m py_compile compatibility/tools/inspect-windows-file.py
	python3 compatibility/tools/inspect-windows-file.py mosaicos.bin >/dev/null || test $$? -eq 1

.PHONY: check-compatibility


test-compatibility:
	python3 -m unittest discover -s compatibility/tests -v

.PHONY: test-compatibility

check-046:
	./tests/validate_046.sh

.PHONY: check-046

check-048: check-050
	@echo "check-048 es un alias histórico; la validación vigente es check-050."

.PHONY: check-048
