#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_
#include "ast.h"

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncType_* FuncType;
typedef struct Sym_* Sym;
typedef struct SymTable_* SymTable;

enum TYPE_ENUM{
    RD_BASIC, 
    RD_ARRAY, 
    RD_STRUCTURE
}; 

struct Type_{
    enum TYPE_ENUM kind;
    union{
        // basic type
        enum BASIC_ENUM basic;
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

enum SYM_ENUM {
    RD_TYPE,
    RD_VARIABLE,
    RD_FUNC
};

struct Sym_{
    char* name;

    enum SYM_ENUM kind;

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

int nameAnalysis(struct ASTNode* root,void* args);
#endif