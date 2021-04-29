#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_
#include "ast.h"

typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;

typedef struct FuncType_ *FuncType;
typedef struct FuncParam_ *FuncParam;

typedef struct Sym_ *Sym;

typedef struct SymTable_ *SymTable;

enum TYPE_ENUM
{
    RD_BASIC,
    RD_STRUCTURE
};

struct Type_
{
    enum TYPE_ENUM kind;
    union
    {
        // basic type
        enum BASIC_ENUM basic;
        // for struct type
        FieldList structure;
    } u;
};

struct FieldList_
{
    char *name;
    Sym type_sym;
    FieldList tail;
};

struct FuncType_
{
    Sym return_type;
    int n_param;
    FuncParam head;
};

struct FuncParam_
{
    Sym type;
    Sym param;
    FuncParam next;
};

enum SYM_ENUM
{
    RD_TYPE,
    RD_VARIABLE,
    RD_FUNC,
    RD_ARRAY
};

struct Sym_
{
    char *name;

    enum SYM_ENUM kind;

    union
    {
        //TYPE
        Type type;
        //VARIABLE
        Sym type_sym;
        //FUNC
        FuncType func_type;
        //ARRAY
        struct
        {
            Sym type_sym;
            int size;
        } array_ty;
    } u;

    //points to the AST node of the sym.
    struct ASTNode *node;

    //next entry in sym table
    Sym next;
};

struct SymTable_
{
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

int nameAnalysis(struct ASTNode *root, void *args);

#endif