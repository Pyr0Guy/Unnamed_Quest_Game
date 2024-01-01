@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -FC -Zi ..\game\src\win32_game.c user32.lib gdi32.lib UxTheme.lib
popd