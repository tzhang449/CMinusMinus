#ifndef _AST_H_
#define _AST_H_

struct ASTNode
{
    int type;
    char name[32];
    int lineno;

    int numKids;
    struct ASTNode *children[10];

    union{
        int int_val;
        float float_val;
        int type_val;
    };

    char str_val[32];
};

enum
{
    SM_Program,
    SM_ExtDefList,
    SM_ExtDef,
    SM_ExtDecList,
    SM_Specifiers,
    SM_StructSpecifier,
    SM_OptTag,
    SM_Tag,
    SM_VarDec,
    SM_FunDec,
    SM_VarList,
    SM_ParamDec,
    SM_StmtList,
    SM_Stmt,
    SM_DefList,
    SM_DecList,
    SM_Dec,
    SM_Exp,
    SM_Args,
    SM_INT,
    SM_FLOAT,
    SM_TYPE,
    SM_ID,
    SM_OTHERTERM
};


struct ASTNode *newNode(char *name, int type, int lineno);

void insert(struct ASTNode *root, struct ASTNode *child);

void printNode(struct ASTNode *root);

#endif