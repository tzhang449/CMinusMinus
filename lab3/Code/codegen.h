#ifndef _CODEGEN_H_
#define _CODEGEN_H_
#include "ast.h"

typedef struct Operand_ *Operand;

struct Operand_
{
    enum
    {
        CG_VARIABLE,
        CG_CONSTANT,
        CG_ADDRESS
    } kind;
    union
    {
        int var_no;
        int value;
    } u;
};

struct InterCode
{
    enum
    {
        CG_FUNCNAME,
        CG_PARAMDEC,
        CG_RETURN,
        CG_LABEL,
        CG_GOTO,

        CG_ASSIGN,
        CG_ADD,
        CG_SUB,
        CG_MUL,
        CG_DIV
    } kind;
    union
    {
        struct
        {
            Operand right, left;
        } assign;
        struct
        {
            Operand result, op1, op2;
        } binop;
        Operand ret;
        int var_no;
        int label_no;
        char *name;
    } u;
};

struct InterCodes{
    struct InterCode code;
    struct InterCodes* pre;
    struct InterCodes* next;
    struct InterCodes* end;
};


struct InterCodes* codeGen(struct ASTNode *root);
struct InterCodes* codeGen_ExtDefList(struct ASTNode* root);
struct InterCodes* codeGen_ExtDef(struct ASTNode* root);
struct InterCodes* codeGen_CompSt(struct ASTNode* root);
struct InterCodes* codeGen_StmtList(struct ASTNode* root);
struct InterCodes* codeGen_Stmt(struct ASTNode* root);
struct InterCodes* codeGen_Exp(struct ASTNode *root, Operand place);
#endif