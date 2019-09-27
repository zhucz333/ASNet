@echo off

set buildDir=%~dp0\..\build_vs2017_32

if not exist %buildDir%	(md %buildDir%)

cd %buildDir%

echo Setup Visual Studio 2017 32 bit Solution in %cd%

cmake -DUSE_32BITS=ON -G "Visual Studio 15" ..

pause