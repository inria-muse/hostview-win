@echo off

setlocal ENABLEDELAYEDEXPANSION
set l=%1
set c=0
set line=0
for /f "delims=" %%1 in ('type ..\include\product.h') do (
	set /a c+=1
	if "!c!" equ "%l%" set line=%%1%
)

for /f "tokens=3" %%G IN ("%line%") DO echo|set /p=%%G