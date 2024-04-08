
@echo off

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

gcc -g -c absyn.c
gcc -g -c symbol.c
gcc -g -c types.c

gcc -g -c temp.c
gcc -g -c myframe.c
gcc -g -c translate.c


gcc -g -c table.c
gcc -g -c parse.c
gcc -g -c parsetest.c
gcc -g -c semant.c
gcc -g -c env.c
gcc -g parse.o tiger.tab.o lex.yy.o errormsg.o util.o table.o absyn.o symbol.o prabsyn.o types.o env.o semant.o temp.o myframe.o translate.o

@REM echo "testing test file ..."
@REM a.exe ".\testfiles\test2.tig"
@REM a.exe ".\testfiles\compilable\array_equality.tig"

set "folder_path=testfiles\compilable"
for /R "%folder_path%" %%F in (*.tig) do (
    echo testing file now : "%%~fF"
    a.exe %%F
)


