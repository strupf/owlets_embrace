@RD /S /Q "%~dp0\game.pdx"

CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd build
nmake

xcopy "..\assets" "..\game.pdx\assets" /E /I

pause