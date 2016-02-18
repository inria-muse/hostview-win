:: Stop Service & Client
start "" "%cd%\HostView.exe" /stop
net stop "HostView Service"
regsvr32 /u /s "hostviewbho.dll"

:: Sleep a bit
ping 127.0.0.1

:: Kill
taskkill /F /IM "HostView.exe"

:: Copy Self-Dir to Current Dir
xcopy /e /h /y /c "%~dp0*.*" "%cd%\"

:: Remove this batch file
del "%cd%\update.bat"

:: Start Service & Client
regsvr32 /s "hostviewbho.dll"
net start "HostView Service"
start "" "%cd%\HostView.exe"

:: Remove Temporary Files
rd /s /q ""%~dp0"