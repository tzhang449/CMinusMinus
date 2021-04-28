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
    SM_ExtDef_SES,
    SM_ExtDef_SS,
    SM_ExtDef_SFC,
    SM_ExtDecList_V,
    SM_ExtDecList_VCE,
    SM_Specifiers_T,
    SM_Specifiers_S,
    SM_StructSpecifier_SOLDR,
    SM_StructSpecifier_ST,
    SM_OptTag,
    SM_Tag,
    SM_VarDec_I,
    SM_VarDec_VLIR,
    SM_FunDec_ILVR,
    SM_FunDec_ILR,
    SM_VarList_PCV,
    SM_VarList_P,
    SM_ParamDec,
    SM_CompSt,
    SM_StmtList,
    SM_Stmt_ES,
    SM_Stmt_C,
    SM_Stmt_RES,
    SM_Stmt_ILERS,
    SM_Stmt_ILERSES,
    SM_Stmt_WLERS,
    SM_DefList,
    SM_Def,
    SM_DecList_D,
    SM_DecList_DCD,
    SM_Dec_V,
    SM_Dec_VAE,
    SM_Exp_ASSIGN,
    SM_Exp_AND,
    SM_Exp_OR,
    SM_Exp_RELOP,
    SM_Exp_PLUS,
    SM_Exp_MINUS,
    SM_Exp_STAR,
    SM_Exp_DIV,
    SM_Exp_LPERP,
    SM_Exp_MINUSE,
    SM_Exp_NOTE,
    SM_Exp_ILPARP,
    SM_Exp_ILPRP,
    SM_Exp_ELBERB,
    SM_Exp_EDI,
    SM_Exp_ID,
    SM_Exp_INT,
    SM_Exp_FLOAT,
    SM_Args_ECA,
    SM_Args_E,
    SM_INT,
    SM_FLOAT,
    SM_TYPE,
    SM_ID,
    SM_OTHERTERM
};


struct ASTNode *newNode(char *name, int type, int lineno);

void insert(struct ASTNode *root, struct ASTNode *child);

#endif