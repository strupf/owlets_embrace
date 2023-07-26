
:: include directories
set "INC=-I src -I src/include"

:: linker flags
set "LIB=-L lib -lraylib -lgdi32 -lopengl32 -lwinmm"

:: preprocessor flags
set "PP_FLAGS=-DTARGET_COMPILED -DTARGET_DESKTOP"

gcc src/main.c %PP_FLAGS% %LIB% %INC% -g -o bin/main

:: extract debug information from gcc generated exe
:: generated .gdb for use in remedybg
cv2pdb bin/main.exe

:: debug with remedybg
remedybg bin/main.exe
pause