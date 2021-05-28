#ifndef _CODEGEN_H_
#define _CODEGEN_H_
#include "ast.h"
#include "stdio.h"

typedef struct Operand_ *Operand;

struct Operand_
{
    enum
    {
        CG_VARIABLE,
        CG_CONSTANT,
        CG_ADDRESS,
        CG_PARAM
    } kind;
    union
    {
        int var_no;
        int value;
        struct{
            int var_no;
            Sym type_sym;
        } addr;
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
        CG_RELOPGOTO,

        CG_ASSIGN,
        CG_ADD,
        CG_SUB,
        CG_MUL,
        CG_DIV,

        CG_READ,
        CG_WRITE,
        CG_CALL,
        CG_ARG,

        CG_ADDRASSIGN,
        CG_ASSIGN_DEREFER,
        CG_ASSIGN_TOADDR,
        CG_ASSIGN_BOTHDEREFER,
        CG_MALLOC
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
        struct
        {
            Operand left, right;
            char *op;
            int label_no;
        } relopgoto;
        struct
        {
            Operand place;
            char *name;
        } func_call;
        struct{
            int var_no;
            int size;
        } malloc;
        Operand ret;
        Operand arg;
        int var_no;
        int label_no;
        char *name;
    } u;
};

struct InterCodes
{
    struct InterCode code;
    struct InterCodes *pre;
    struct InterCodes *next;
    struct InterCodes *end;
};

void print_codes(struct InterCodes *codes, FILE *file);

struct InterCodes *codeGen(struct ASTNode *root);
struct InterCodes *codeGen_ExtDefList(struct ASTNode *root);
struct InterCodes *codeGen_ExtDef(struct ASTNode *root);
struct InterCodes *codeGen_CompSt(struct ASTNode *root);
struct InterCodes *codeGen_StmtList(struct ASTNode *root);
struct InterCodes *codeGen_Stmt(struct ASTNode *root);
struct InterCodes *codeGen_Exp(struct ASTNode *root, Operand place);
struct InterCodes *codeGen_Args(struct ASTNode *root, Operand *arg_list, int i);
struct InterCodes *codeGen_DefList(struct ASTNode *root);
struct InterCodes *codeGen_DecList(struct ASTNode *root);
struct InterCodes *codeGen_Dec(struct ASTNode *root);
struct InterCodes *codeGen_VarDec(struct ASTNode *root);
struct InterCodes *gen_arrayAssign(Operand left,Operand right);
#endif