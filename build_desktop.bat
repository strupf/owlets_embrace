
:: include directories
set "INC=-I src -I src/include"

:: linker flags
set "LIB=-L lib/mingw -lraylib -lgdi32 -lopengl32 -lwinmm"

:: preprocessor flags
set "P_FLAGS=-DTARGET_COMPILED -DTARGET_DESKTOP"

:: warnigns enable
set "NOWARN=-Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-missing-braces"

:: compiler flags
set "C_FLAGS=-g -Wall -Wextra"

gcc src/all.c %P_FLAGS% %LIB% %INC% %C_FLAGS% %NOWARN% -o bin/gcc/all

:: extract debug information from gcc generated exe
:: generated .gdb for use in remedybg
cv2pdb bin/gcc/all.exe

:: debug with remedybg
remedybg bin/gcc/all.exe
pause