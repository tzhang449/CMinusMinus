#include "stdio.h"

#include "ast.h"
#include "semantic.h"
#include "codegen.h"

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
    //print_node(root);
    if(nameAnalysis(root,NULL)){
        //error
        return 0;
    }

    struct InterCodes* codes=codeGen(root);
    
    FILE *file_out = fopen(argv[2], "w+");
    print_codes(codes,file_out);

    return 0;
}