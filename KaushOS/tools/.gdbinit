target remote localhost:1234
set disassembly-flavor intel
symbol-file obj/kern/kernel
set print pretty on
source ./.gdbscript
