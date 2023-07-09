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


.bat 代码:

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


set "folder_path=D:\projs\test\C\tiger\testcases"

for /R "%folder_path%" %%F in (*) do (

a.exe %%F

)
