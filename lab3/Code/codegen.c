#include "codegen.h"
#include "semantic.h"
struct InterCodes program;
SymTable variables;

void codeGen(struct ASTNode *root)
{
    symTable_print(variables);
    //codeGen_ExtDefList(root->children[0]);
}

void codeGen_ExtDefList(struct ASTNode* root){
    codeGen_ExtDef(root->children[0]);
    codeGen_ExtDefList(root->children[1]);
}

void codeGen_ExtDef(struct ASTNode* root){
    switch (root->type)
    {
    case SM_ExtDef_SFC:
        codeGen_CompSt(root->children[2]);
        /* code */
        break;    
        
    default:
        break;
    }
}

void codeGen_CompSt(struct ASTNode* root){

}