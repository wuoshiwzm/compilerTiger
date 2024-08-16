
[assem.h](assem.h): 汇编语句数据结构



1. 实现 codegen.h 中的 xxxcodegen.c, 用 max munch 转换算法将 IR 树转换为 Assem 数据结构  （xxx 代表 Sparc, Mips, Alpha, Pentium, etc.）
2. 使用第8章的 Canon 模块简化
3. 使用函数 AS_printInstrList 将 codegen 得到的 Assem 树转换为 RISC 汇编语言。
4. 因为还没有进行寄存器分配，只要将 Temp_name 传递给 AS_print ，把临时变量转换到字符串。
