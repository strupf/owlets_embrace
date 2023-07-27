# PlaydateMV
The goal is to make an open source 2D metroidvania from scratch using almost no external libraries for the Playdate in C. I only upload the source code and keep stuff like lib files and assets local.

# Build

For simplicity reasons a unity build is used: all.c is going to include all other .c files to produce the final build. The project is going to have two backends to help with debugging and playtesting:
- **Playdate** is the target hardware which will provide all the design and performance constraints for the game.
- **Raylib** will provide some kind of emulation layer especially for the input and display for the desktop.
