@ECHO OFF

ECHO HostView Installation
set /p userId=Input user id: 

if "%PROCESSOR_ARCHITECTURE%"=="AMD64" goto 64BIT

:: 32 bit Installation
ECHO Removing for %userId% on 32-bit OS

mkdir %PROGRAMFILES%\HostView
xcopy win32\*.* "%PROGRAMFILES%\HostView"

goto END

:: 64 bit Installation
:64BIT
ECHO Installing for %userId% on 64-bit OS

mkdir "%PROGRAMFILES%\HostView"
xcopy x64\*.* "%PROGRAMFILES%\HostView"

:END

:: Common Stuff
:: settings file
set settingsFile="%PROGRAMFILES%\HostView\settings"
(
echo enduser = %userId%
echo interfaceMonitorTimeout = 250
echo autoSubmitInterval = 21600000
echo userMonitorTimeout = 1000
echo userIdleTimeout = 5000
echo socketStatsTimeout = 60000
echo systemStatsTimeout = 60000
echo pcapSizeLimit = 5242880 
echo dbSizeLimit = 5242880
) > %settingsFile%

:: Install Service
"%PROGRAMFILES%\HostView\hostviewcli.exe" /install

:: Install Client
REG ADD "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "HostView" /t REG_SZ /d "%PROGRAMFILES%\HostView\HostView.exe"

:: Start Service & Client
net start "HostView Service"
start "" "%PROGRAMFILES%\HostView\HostView.exe" /start

echo Installation done.