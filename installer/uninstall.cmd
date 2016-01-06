@ECHO OFF

ECHO HostView Uninstaller

:: Stop Service & Client
net stop "HostView Service"
taskkill /F /IM "HostView.exe"

:: uninstall service
"%PROGRAMFILES%\HostView\hostviewcli.exe" /remove

:: uninstall client
REG DELETE "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "HostView" /f

:: removing all files
rmdir /S /Q "%PROGRAMFILES%\HostView\"

echo All has been removed.
