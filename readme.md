# PlaydateMV
This is my open source project for the Playdate in C. The goal is to make a 2D metroidvania from scratch using almost no external libraries. Currently I only upload the source code and keep stuff like lib files and assets local.

# Build

For simplicity reasons a unity build is used: all.c is going to include all other .c files to produce the final build. The project is going to have two backends to help with debugging and playtesting:
- **Playdate** is the target hardware which will provide all the design and performance constraints for the game. It's build by following the instructions in official PD documentation: https://sdk.play.date/2.0.1/Inside%20Playdate%20with%20C.html.
- **Raylib** will provide some kind of emulation layer especially for the input and display for the desktop. The raylib target builds with GCC and a simple .bat file.
