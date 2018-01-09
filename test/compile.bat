@echo off

cd %~dp0

echo Compiling with G++...
g++ -std=c++11 -I"../include" -O2 -Wall -o test.exe *.cpp

IF %ERRORLEVEL% NEQ 0 GOTO ERROR

echo ...done & echo.

:ERROR

echo.

pause