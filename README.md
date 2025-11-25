## A tiny hobby kernel written by me.

### what it does:
- Boots on qemu and displays "Kernel Booted Successfully!" on the screen.
- A simple VGA text mode driver to print text to the screen.

### Build Requirements
- x86_64-elf-gcc (cross-compiler)
- nasm
- qemu-system-x86_64
- make

### Building and Running
1. Clone the repository:
   ```bash
   git clone https://lilcloudcoder/kernel.git
   cd kernel
   ```
2. Build the kernel:
   ```bash
   make
   ```
3. Run the kernel in QEMU:
   ```bash
   make run
   ```
### It was made on `MacBook m2 air` with `macOS Tahoe 26 beta 3`

