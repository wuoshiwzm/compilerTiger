/*
 * types.h - 类型结构
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

typedef struct Ty_ty_ *Ty_ty;
typedef struct Ty_tyList_ *Ty_tyList;
typedef struct Ty_field_ *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

/* 类型结构体 */
struct Ty_ty_ {
    enum {
        /* 基础类型 */
        Ty_record, Ty_nil, Ty_int, Ty_string, Ty_array, Ty_name, Ty_void, Ty_double
    } kind;
    union {
        Ty_fieldList record;
        Ty_ty array;
        struct {
            S_symbol sym;
            Ty_ty ty;
        } name;
    } u;
};

struct Ty_tyList_ {Ty_ty head; Ty_tyList tail;};
struct Ty_field_ {S_symbol name; Ty_ty ty;};
struct Ty_fieldList_ {Ty_field head; Ty_fieldList tail;};

/* 初始化基础类型 nil, int, string, double */
Ty_ty Ty_Nil(void);
Ty_ty Ty_Int(void);
Ty_ty Ty_String(void);


/* void 类型 */
Ty_ty Ty_Void(void);
Ty_ty Ty_Double(void);
/* 域列表类型 */
Ty_ty Ty_Record(Ty_fieldList fields);
/* 数组类型 */
Ty_ty Ty_Array(Ty_ty ty);

/* 自定义名称（name）的类型 */
Ty_ty Ty_Name(S_symbol sym, Ty_ty ty);

/* 类型列表（链表） */
Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail);
/* 域类型 */
Ty_field Ty_Field(S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail);

void Ty_print(Ty_ty t);
void TyList_print(Ty_tyList list);
