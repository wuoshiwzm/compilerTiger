
@echo off

del tiger.output
del tiger.tab.*
del *.o
del lex.yy.c
del *.tab.*
del a.out
del a.exe

flex tiger.lex
bison -dv tiger.y
gcc -g -c lex.yy.c
gcc -g -c tiger.tab.c

gcc -g -c util.c
gcc -g -c symbol.c
gcc -g -c types.c
gcc -g -c absyn.c
gcc -g -c temp.c
gcc -g -c tree.c
gcc -g -c table.c
gcc -g -c env.c

gcc -g -c errormsg.c
gcc -g -c prabsyn.c
gcc -g -c printtree.c

gcc -g -c semant.c
gcc -g -c parse.c
gcc -g -c translate.c
gcc -g -c assem.c
gcc -g -c frame.c
gcc -g -c canon.c
gcc -g -c mycodegen.c
gcc -g -c graph.c
gcc -g -c flowgraph.c
gcc -g -c liveness.c
gcc -g -c main.c


gcc -g main.o parse.o tiger.tab.o lex.yy.o errormsg.o util.o table.o absyn.o symbol.o prabsyn.o types.o env.o semant.o temp.o tree.o printtree.o frame.o translate.o assem.o canon.o mycodegen.o graph.o flowgraph.o liveness.o

echo ">>> Start testing files ..."
a.exe "..\testfiles\test37.tig"

@REM set "folder_path=testfiles\compilable"
@REM for /R "%folder_path%" %%F in (*.tig) do (
@REM   echo testing file now : "%%~fF"
@REM   a.exe %%F
@REM )

