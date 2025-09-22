qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a55 \
  -nographic -smp 2 \
  -hda ./rootfs.ext2 \
  -kernel ./kernel/arch/arm64/boot/Image \
  -append "console=ttyAMA0 root=/dev/vda nokaslr quiet" \
  -m 2048 \
  -net user,hostfwd=tcp::10023-:22 -net nic \
  -s
