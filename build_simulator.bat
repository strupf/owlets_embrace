:: delete old playdate file
@RD /S /Q "%~dp0\game.pdx"

:: init vs environment to use nmake
CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd build
nmake

:: go back into root folder
cd ..

:: copy game asset folder into the playdate file
xcopy "assets" "game.pdx\assets" /E /I

:: run game
C:\PlaydateSDK\bin\PlaydateSimulator.exe game.pdx

pause