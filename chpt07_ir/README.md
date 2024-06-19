# 翻译成 tree 语言

本节介绍中间代码环节，
Tree 模块将抽象语法 Absyn 转为 Tree 语法，
Frame 保存栈帧， 
Translate 模块将 T_exp, T_stm 等语句/表达式 转化为 Tr_exp, 继尔转化中间代码格式的片段：fragList
Semant 模块调用 Translate，最终将抽象语法 A_exp 翻译成片段 F_fragList 。

1) 修改 Translate 模块
2) 使用提供的 Tree 模块
3) Semant 正确调用 Translate 模块，调用 SEM_transProg() 产出片段 F_fragList

简单起见，
1) 所有局部变量都存在栈帧内
2) 不调用FindEscape, 假定每一个变量都是逃逸的
3) 栈帧实现基本不变
4) Frame 模块中暂时使用空实现，通过测试：
    T_stm F_procEntryExit1(F_frame frame, T_stm stm){
        return stm;
    }
5) 提供文件 printtree.c ，调试打印树结构
6) 朴素实现： Translate 模块：
```c
// 不使用构造函数 Tr_Ex Tr_Nx Tr_Cx
不使用 Tr_Ex(T_exp e) => 使用  e
不使用 Tr_Nx(T_stm s) => 使用 ESEQ(s, CONST 0)
不使用 Tr_Cx(...)     => 使用计算结果为 1 或 0 的一个表达式
```

