# 第五章 语义分析


bat run test:
```

@echo off

echo "start parsing "

del tiger.output
del tiger.tab.*
del *.o
del lex.yy.c
del *.tab.*

del a.out 


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
gcc -g -c parse.c
gcc -g -c semant.c
gcc -g -c env.c
gcc -g -c types.c
 
 
gcc -g parse.o tiger.tab.o lex.yy.o errormsg.o util.o table.o absyn.o symbol.o prabsyn.o semant.o types.o env.o

echo "testing test file ..."

set "folder_path=D:\projs\test\C\tiger\testcases"

for /R "%folder_path%" %%F in (*) do (
    a.exe %%F
)

cd ..


```






### https://rebelsky.cs.grinnell.edu/Courses/CS362/98F/Outlines/outline.26.html
### https://github.com/oyzh/tiger/blob/master/chap5/semant.c
### https://www.lrde.epita.fr/~tiger/tc-doc/classtype_1_1Record.html

### 语义分析，类型检查
```

```
semant.c
semant.h

### 抽象语法
```

```
absyn.c
absyn.h


### 符号
```

```
symbol.c
symbol.h


### 符号表
```

```
table.c
table.h


### 类型
```

```
types.c
types.h



### 环境相关函数，


```
环境 对应 符号表，
有值环境 venv，类型环境 tenv
值环境分两种， 变量环境 与 函数环境
变量环境的构造函数 E_varEntry 
函数环境的构造函数 E_funEntry

提供创建环境的构造函数
创建初始化环境
初始化类型环境： 包括语言的基本类型， int, string  等
初始化函数环境： 包括语言提供的库函数， print(), getchar() 等

```
env.c
env.h

errormsg.c
errormsg.h


makefile

### 解析
parser.c
parser.h
parsetest.c
parsetest.h


### 语法 & 语义
tiger.grm   :  bison (yacc)
tiger.lex   :  flex  (lex)
translate.h


### 工具类
util.c
util.h

