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
        CG_ASSIGN,
        CG_ADD,
        CG_SUB,
        CG_MUL
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
    } u;
};

struct InterCodes{
    struct InterCode code;
    struct InterCode* pre;
    struct InterCode* next;
};

void codeGen(struct ASTNode *root);
void codeGen_ExtDefList(struct ASTNode* root);
void codeGen_ExtDef(struct ASTNode* root);
void codeGen_CompSt(struct ASTNode* root);
#endif