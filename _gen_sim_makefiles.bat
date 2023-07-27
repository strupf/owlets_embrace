:: delete build folder and recreate it
@RD /S /Q "%~dp0\build"
mkdir build

:: init vs environment
:: run cmake to create nmake makefiles
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd build
cmake .. -G "NMake Makefiles"

pause