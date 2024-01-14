
@echo off

echo "start parsing "

del tiger.output
del tiger.tab.*
del *.o
del lex.yy.c
del *.tab.*
del a.out
del a.exe

gcc -g -c util.c
flex tiger.lex
bison -dv tiger.y

gcc -g -c lex.yy.c
gcc -g -c tiger.tab.c

gcc -g -c errormsg.c
gcc -g -c prabsyn.c

gcc -g -c table.c
gcc -g -c absyn.c
gcc -g -c symbol.c
gcc -g -c parser.c
gcc -g -c semant.c
gcc -g -c env.c
gcc -g -c types.c


gcc -g parser.o tiger.tab.o lex.yy.o errormsg.o util.o table.o absyn.o symbol.o prabsyn.o semant.o types.o env.o

echo "testing test file ..."

@REM set "folder_path=D:\projs\test\C\tiger\testcases"
set "folder_path=.\testfiles"

for /R "%folder_path%" %%F in (*.tig) do (
    echo "testing file now :::"
    echo %%F
    a.exe %%F
)

cd ..
