@echo off
set LOCALHOST=%COMPUTERNAME%
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 4704)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 16940)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 13780)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 13116)

del /F cleanup-ansys-Spooky_Laptop-13116.bat
