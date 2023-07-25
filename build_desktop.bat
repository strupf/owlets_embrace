set "CC=gcc"
set "INC=-I src -I src/include"
set "LIB=-L lib -lraylib -lgdi32 -lopengl32 -lwinmm"

%CC% src/main.c %LIB% %INC% -g -o bin/main
cv2pdb bin/main.exe
remedybg bin/main.exe
pause