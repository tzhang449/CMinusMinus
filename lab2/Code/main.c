#include "stdio.h"
#include "stdlib.h"

#include "ast.h"
#include "semantic.h"

extern int yyrestart();
extern int yyparse();

struct ASTNode* root=NULL;
int hasError=0;

int main(int argc, char **argv)
{
    if (argc <= 1)
        return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }

    yyrestart(f);
    yyparse();
    
    if(hasError)
        return 0;
    
    hasError=0;
    SymTable* globalSymTable=(SymTable*)malloc(sizeof(struct SymTable_));
    nameAnalysis(root);


    return 0;
}