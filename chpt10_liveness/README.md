
[assem.h](assem.h): 汇编语句数据结构

1. 实现 codegen.h 中的 xxxcodegen.c, 用 max munch 转换算法将 IR 树转换为 Assem 数据结构  （xxx 代表 Sparc, Mips, Alpha, Pentium, etc.）
2. 使用第8章的 Canon 模块简化
3. 使用函数 AS_printInstrList 将 codegen 得到的 Assem 树转换为 RISC 汇编语言。
4. 因为还没有进行寄存器分配，只要将 Temp_name 传递给 AS_print ，把临时变量转换到字符串。


# 整体代码流程：

-> 编译器入口  main()
初始化全局变量，如语法树根，代码段列表


-> 抽象语法树  absyn.c(A_xxx)
absyn_root = parse()  
代码解析，生成抽象语法树,并返回树根


-> 翻译中间代码 (IR树)  tree.c(T_xxx), translate.c(Tr_xxx )
F_fragList frags = SEM_transProg(absyn_root)  
1. 解析语法树（从树根开始），按不同语法节点对应，生成中间代码IR树 T_Move, T_Mem, ...  -> T_exp, T_expList, T_stm, T_stmList
2. 翻译 T_exp, T_stm -> Tr_exp;  Tr_exp -> T_exp, T_stm
3. 最终生成代码片段列表及栈帧:  F_fragList Tr_getResult(void); 


-> 遍历代码片段，生成汇编指令 (AS_instrList) , 写入目标文件  codegen_risc.c
1. 基本块  stmList = C_linearize(T_stm body); stmList = C_traceSchedule(C_basicBlocks(stmList));
2. 解析语句，生成汇编指令，iList  = F_codegen(frame, stmList)
3. 遍历每一个句语，通过 emit(AS_instr instr) 更新全局汇编指令树 AS_instrList iList
4. 将生成的汇编指令写入生成的目标文件 AS_printInstrList()

# 主要代码


-> [canon.c](canon.c)
使用基本块简化 IR 树

-> frame.h, myframe.c
```c
    // 将IR语句转化为汇编语句
    AS_instrList F_codegen(F_frame f, T_stmList stmList);

    // 给函数体添加一个 “下沉” 的指令，告诉寄存器分配器 那些寄存器在流程结束时还是生效的。（即 0寄存器，返回值，返回地址，调用者要保存的临时变量）
    AS_instrList F_procEntryExit2(AS_instrList body);

    // 本章的代码生成只处理函数体,函数的进入 和 退出业务由此函数处理
    AS_proc F_procEntryExit3(F_frame frame, AS_instrList body)
```



-> [myframe.c](frame.c)

```c
// 1. 初始化寄存器 ，如返回值 rv, 栈帧 fp, 栈指针 sp 等
static Temp_temp fp; // fp = Temp_newtemp();
static Temp_temp ra;
static Temp_temp rv;
static Temp_temp sp;
static Temp_temp zero;

// 2. 初始化 SN、TN、AN 和 VN，通常代表不同的编译器内部符号或标识符
//    SN：Symbol Name（符号名称），表示编译器中的一个符号名称。
//    TN：Type Name（类型名称），表示编译器中的一个类型名称。
//    AN：Abstract Name（抽象名称），表示编译器中的一个抽象名称，可能用于命名空间或其他抽象概念。
//    VN：Variable Name（变量名称），表示编译器中的一个变量名称。
static Temp_temp sn[8] = {Temp_newtemp(), ...};
static Temp_temp tn[10] = {Temp_newtemp(), ...};
static Temp_temp an[4] = {Temp_newtemp(), ...};
static Temp_temp vn[2] = {Temp_newtemp(), ...};

// 3. 将之前生成的寄存器地址 映射对应的字符串，如 F_FP() -> String("$fp"))
static Temp_map _F_tempMap;// 寄存器地址全局映射表
Temp_map F_tempMap();


// 4. 给函数体添加一个 “下沉” 的指令，告诉寄存器分配器 那些寄存器在流程结束时还是生效的。（即 0寄存器，返回值，返回地址，调用者要保存的临时变量）
static AS_instrList F_procEntryExit2(AS_instrList body)


// 5. 本章的代码生成只处理函数体，函数的进入 和 退出业务由此函数处理
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

```




-> [codegen_risc.c](codegen_risc.c)
汇编指令生成

```c
// 全局汇编指令表
static AS_instrList iList = NULL, iList_last = NULL;

// IR 树转化为汇编指令
static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);
static void emit(AS_instr instr);

// 把所有实在参数传递到正确位置
static Temp_tempList munchArgs(int i, T_expList args, int *stack_size);

// 栈帧转化为汇编指令
AS_instrList F_codegen(F_frame frame, T_stmList stmlist)


```


[assem.c](assem.c)


```c
// 将汇编语句树转化为汇编指令输出
AS_printInstrList (FILE *out, AS_instrList iList, Temp_map m);
```

```c

```


# MIPS instruction references


```
MIPS instructions			Assembly code		Meaning
Add 					add $d,$s,$t 		$d = $s + $t
Subtract 				sub $d,$s,$t 		$d = $s - $t
Add immediate 				addi $t,$s,C 		$t = $s + C (signed)
Load word 				ld $t,C($s) 		$t = Memory[$s + C]
Store word 				sw $t,C($s) 		Memory[$s + C] = $t
And 					and $d,$s,$t 		$d = $s & $t
And immediate 				andi $t,$s,C 		$t = $s & C
Or 					or $d,$s,$t 		$d = $s | $t
Or immediate 				ori $t,$s,C 		$t = $s | C
Shift left logical 			sll $d,$t,shamt 	$d = $t << shamt
Shift right logical 			srl $d,$t,shamt 	$d = $t >> shamt
Shift right arithmetic 			sra $d,$t,shamt
Branch on equal 			beq $s,$t,C 		if ($s == $t) go to PC+4+4*C
Branch on not equal 			bne $s,$t,C 		if ($s != $t) go to PC+4+4*C
Jump 					j C 			PC = PC+4[31:28] . C*4
Jump register 				jr $s 			goto address $s
Jump and link 				jal C 			$31 = PC + 8; PC = PC+4[31:28] . C*4
(For procedure call - used to call a subroutine, $31 holds the return address; returning from
a subroutine is done by: jr $31. Return address is PC + 8, not PC + 4 due to the use of a
branch delay slot which forces the instruction after the jump to be executed)
Move 					move $rt,$rs 		addi $rt,$rs,0
Branch if greater than 			bgt $rs,$rt,Label 	
Branch if less than 			blt $rs,$rt,Label 	
Branch if greater than or equal 	bge $rs,$rt,Label 	
Branch if less than or equal 		ble $rs,$rt,Label
Multiplies and returns only first 32 bits 	mul $d, $s, $t 	
Divides and returns quotient 		div $d, $s, $t 	
```








