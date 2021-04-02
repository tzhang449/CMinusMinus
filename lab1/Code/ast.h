#ifndef _AST_H_
#define _AST_H_

struct ASTNode
{
    int type;
    char name[32];
    int lineno;

    int numKids;
    struct ASTNode *children[10];

    int int_val;
    float float_val;
    char str_val[32];
    int type_val;
};

enum
{
    ISOTHER,
    ISINT,
    ISFLOAT,
    ISID,
    ISTYPE,
    ISOTHERTERM
};

struct ASTNode *newNode(char *name, int type, int lineno);

void insert(struct ASTNode *root, struct ASTNode *child);

void printNode(struct ASTNode *root);

#endif