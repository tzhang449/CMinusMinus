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

    if (root->numKids >= 10)
    {
        printf("ASTnode insert error: maximum children num execeeded!\n");
        return;
    }
    root->children[root->numKids] = child;
    root->numKids++;
}

void print_node(struct ASTNode *root){
    if(!root)
        return;
    static int indent=0;
    indent+=2;
    for(int i=0;i<indent;i++){
        printf(" ");
    }
    printf("%s\n",root->name);
    for(int i=0;i<root->numKids;i++){
        print_node(root->children[i]);
    }
    indent-=2;
}
