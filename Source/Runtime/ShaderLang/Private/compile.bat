@echo off
set str=%1
set tool_dir=..\..\..\ThirdParty\GnuWin32\bin\
set current_dir=%cd%
REM @echo on
cd %tool_dir%
.\bison.exe --yacc -dv --output=%current_dir%\output\y.tab.c --defines=%current_dir%\output\y.tab.h %current_dir%\%str%.y
.\flex.exe -o%current_dir%\output\lex.yy.c %current_dir%\%str%.l
gcc -o %current_dir%\%str% %current_dir%\output\y.tab.c %current_dir%\output\lex.yy.c