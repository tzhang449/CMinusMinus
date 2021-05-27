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
    RD_ARRAY,
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
        // Array
        struct
        {
            Sym elem;
            //type_sym of the next level array
            Sym next;
            int total_size;
            int size;
            int dim;
        } array_ty;
    } u;
    int size;
};

struct FieldList_
{
    char *name;
    Sym sym;
    FieldList tail;
    int offset;
};

struct FuncType_
{
    Sym return_type;
    int n_param;
    FuncParam head;
    int defined;
    int lineno;
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
    RD_ARRAY_VARIABLE
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
    } u;

    //next entry in sym table
    Sym next;
    int var_no;
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
void symTable_print(SymTable table);
Sym symTable_find(SymTable table, char *name, enum SYM_ENUM kind);
#endif