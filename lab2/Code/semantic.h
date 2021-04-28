#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_
#include "ast.h"

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncType_* FuncType;
typedef struct Sym_* Sym;
typedef struct SymTable_* SymTable;

struct Type_{
    enum type_type{
        RD_BASIC, 
        RD_ARRAY, 
        RD_STRUCTURE} kind;
    union{
        // basic type
        enum TYPE_ENUM basic;
        // for array type
        struct{
            Type elem;
            int size;
        } array;
        // for struct type
        FieldList structure;
    } u;
};

struct FieldList_{
    char* name;
    Type type;
    FieldList tail;
};

struct FuncType_{
    Sym return_type;
    int n_param;
    Sym* param_type_sym;
};

struct Sym_{
    enum sym_type {
        RD_TYPE,
        RD_VARIABLE,
        RD_FUNC
    } kind;

    union{
        //TYPE
        Type type;
        //VARIABLE
        Sym type_sym;
        //FUNC
        FuncType func_type;
    } u;

    //points to the AST node of the sym.
    struct ASTNode *node;

    //next entry in sym table
    Sym next;
};

struct SymTable_{
    //head element of the sym table
    Sym head;
    //for Sym stack
    SymTable pre;
    SymTable next;
};

/*
Type makeType();
FuncType makeFuncType();
Sym makeSym(enum sym_type kind);
SymTable makeSymTable();
*/

void nameAnalysis(struct ASTNode* root,void* args);
#endif