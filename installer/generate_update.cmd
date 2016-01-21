
rd /s /q latest

:: Packs everything in one update zip file for each platform
mkdir latest
7za.exe a latest\w32.zip ..\bin\win32\release\*.dll ..\bin\win32\release\*.exe ..\src\HostView\html update.bat
7za.exe a latest\w64.zip ..\bin\x64\release\*.dll ..\bin\x64\release\*.exe ..\src\HostView\html update.bat
makensis hostview.nsi
move hostview_installer.exe latest\hostview_installer.exe
version.bat 5 > latest\version