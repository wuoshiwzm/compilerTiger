@echo off
bison -dv tiger.y
gcc -g -c util.c
flex tiger.lex
gcc -g -c lex.yy.c
gcc -g -c errormsg.c
gcc -g -c tiger.tab.c
gcc -g -c parsetest.c
gcc -g parsetest.o tiger.tab.o lex.yy.o errormsg.o util.o
echo "parsing test file ..."
a.exe test.file
