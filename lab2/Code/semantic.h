#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct FuncType_* FuncType;
typedef struct Sym_* Sym;
typedef struct SymTable_ SymTable;

struct Type_{
    enum {
        BASIC, 
        ARRAY, 
        STRUCTURE} kind;
    union{
        // basic type
        int basic;
        // for array type
        struct{
            Type elem;
            int size;
        } array;
        // for struct type
        FiledList structure;
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
}

struct Sym_{
    enum {
        TYPE,
        VARIABLE,
        FUNC
    };

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
    SymTable* pre;
    SymTable* next;
};


void nameAnalysis(ASTNode* root);
#endif