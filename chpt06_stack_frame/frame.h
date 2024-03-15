/*
    栈帧存什么？    形参， 局部变量
    栈帧在哪里？    SP + offset
    帧指针是什么？  就是SP, 栈指针
    什么时候决定放寄存器/栈帧？ 一开始统一存放在 临时位置，晚些再决定哪些变量放在寄存器中，哪些放在栈帧中
*/

/*  定义类型
    F_frame: 栈帧
    F_access: "访问"， 这个访问可以是形参，也可以是局部变量
        access to formal: 形参  
        access to alloclocal: 局部变量
*/
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

// 定义+声明
struct F_accessList_ {F_access head; F_accessList tail;}

/*  构造函数，返回一个栈帧对象
    name ： 栈帧标识符
    formals ： 形参是否逃逸

    F_newFrame(g, U_BoolList(True, U_BoolList(False, U_BoolList(False, NULL))))
*/
F_frame F_newFrame(Temp_label name, U_boolList formals)；

// 生成栈帧的标识符
Temp_label F_name(F_frame f);

// 获取函数形参的访问 （注意这里是 list， 因为有多个形参）
F_accessList F_formals(F_frame f);

// 获取对某个局部变量的访问 escape表示是否逃逸
F_access F_allocLocal(F_frame f, bool escape);

