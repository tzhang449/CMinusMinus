#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "ast.h"

struct ASTNode *newNode(char *name, int type, int lineno)
{
    struct ASTNode *ret = (struct ASTNode *)malloc(sizeof(struct ASTNode));

    if (!ret)
    {
        printf("create new node error : malloc failed");
        return NULL;
    }

    strcpy(ret->name, name);
    ret->type = type;
    ret->lineno = lineno;
    ret->numKids = 0;

    return ret;
}

void insert(struct ASTNode *root, struct ASTNode *child)
{
    if (!root)
    {
        printf("ASTnode insert error: root node does not exist!\n");
        return;
    }
    if (!child)
    {
        return;
    }

    if (root->numKids >= 10)
    {
        printf("ASTnode insert error: maximum children num execeeded!\n");
        return;
    }
    root->children[root->numKids] = child;
    root->numKids++;
}

void printIndent(int n)
{
    for (; n--; n > 0)
    {
        printf(" ");
    }
}

void printNode(struct ASTNode *root)
{
    static int indent = 0;
    if (!root)
        return;
    printIndent(indent * 2);
    switch (root->type)
    {
    case ISOTHER:
        printf("%s (%d)\n", root->name, root->lineno);
        break;
    case ISINT:
        printf("INT: %d\n", root->int_val);
        break;
    case ISFLOAT:
        printf("FLOAT: %f\n", root->float_val);
        break;
    case ISID:
        printf("ID: %s\n", root->str_val);
        break;
    case ISTYPE:
        printf("TYPE: %s\n", root->type_val == 0 ? "int" : "float");
        break;
    case ISOTHERTERM:
        printf("%s\n", root->name);
        break;
    default:
        printf("print node error: type error!\n");
        break;
    }
    for (int i = 0; i < root->numKids; i++)
    {
        
        indent++;
        printNode(root->children[i]);
        indent--;
    }
}