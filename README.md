Chip8
=====

Pretty basic CHIP 8 emulator written in C.

Instructions
============
You need SDL library to run this program. Compile emulator using `gcc chip8.c -o chip8 -lSDL`.

Key bindings
============
You can modify all key bindings in chip8.h file.

Key | Action
----|-------
p   | Pause/unpause
b   | Toggle debugging
i   | Output emulator info (registers, etc.) to the console
n   | Execute next instruction (while in pause mode)
h   | Halt the emulator

Keyboard
========
Chip8 was using hex keyboard, which isn't available now. Instead use 1-4, q-r, a-f, z-v for input.

Bugs & problems
===============
Please report any bugs & problems in the issues section! This will help me improve the emulator.
