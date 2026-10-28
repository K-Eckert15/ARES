@echo off
set LOCALHOST=%COMPUTERNAME%
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 23980)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 26172)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 7312)
if /i "%LOCALHOST%"=="Spooky_Laptop" (taskkill /f /pid 24216)

del /F cleanup-ansys-Spooky_Laptop-24216.bat
