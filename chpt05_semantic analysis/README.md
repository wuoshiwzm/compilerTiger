# 第五章 语义分析


### https://rebelsky.cs.grinnell.edu/Courses/CS362/98F/Outlines/outline.26.html
### https://github.com/oyzh/tiger/blob/master/chap5/semant.c
### https://www.lrde.epita.fr/~tiger/tc-doc/classtype_1_1Record.html

# 语义分析，类型检查
```

```
semant.c
semant.h

# 抽象语法
```

```
absyn.c
absyn.h


# 符号
```

```
symbol.c
symbol.h


# 符号表
```

```
table.c
table.h


# 类型
```

```
types.c
types.h



# 环境相关函数，


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

# 解析
parser.c
parser.h
parsetest.c
parsetest.h





tiger.grm
tiger.lex
translate.h

util.c
util.h
