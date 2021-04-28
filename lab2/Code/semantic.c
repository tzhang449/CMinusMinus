#include "stdlib.h"
#include "stdio.h"

#include "semantic.h"

Sym sym_int,sym_float;

extern SymTable global;
extern int hasError;



Type makeType(){
    return (Type)malloc(sizeof(struct Type_));
}

void fillBasicType(Type type, enum TYPE_ENUM basic){
    type->kind=RD_BASIC;
    type->u.basic=basic;
}

FuncType makeFuncType(){
    return (FuncType)malloc(sizeof(struct FuncType_));
}

Sym makeSym(enum sym_type kind){
    Sym ret=(Sym)malloc(sizeof(struct Sym_));
    ret->kind=kind;
    switch(ret->kind){
        case RD_TYPE:
            ret->u.type=makeType();
            break;
        case RD_FUNC:
            ret->u.func_type=makeFuncType();
            break;
        default:
            ret->u.type_sym=NULL;
            //for VARIABLE, the analysier should link it to a type sim
            break;
    }
    ret->node=NULL;
    ret->next=NULL;
    return ret;
}

SymTable makeSymTable(){
    SymTable ret=(SymTable)malloc(sizeof(struct SymTable_));
    ret->head=NULL;
    ret->next=NULL;
    ret->pre=NULL;
    return ret;
}

void nameAnalysis(struct ASTNode* root, void* args){
    if(root==NULL){
        return;
    }

    switch(root->type){
        case SM_Program:
            sym_int=makeSym(RD_TYPE);
            fillBasicType(sym_int->u.type,RD_INT);
            
            sym_float=makeSym(RD_TYPE);
            fillBasicType(sym_int->u.type,RD_FLOAT);

            global=makeSymTable();
            nameAnalysis(root->children[0], NULL);
            break;

        case SM_ExtDefList:
            nameAnalysis(root->children[0], NULL);
            nameAnalysis(root->children[1], NULL);
            break;

        case SM_ExtDef_SES:
            {
                //we need sym of the specifier
                Sym specifier=NULL;
                nameAnalysis(root->children[0],(void*)&specifier);
                nameAnalysis(root->children[1],(void*)specifier);
            }
            break;

        case SM_ExtDef_SS:
            nameAnalysis(root->children[0],NULL);
            break;

        case SM_ExtDef_SFC:
            {
                //we need sym of the specifier
                Sym specifier=NULL;
                nameAnalysis(root->children[0],(void*)&specifier);
                nameAnalysis(root->children[1],(void*)specifier);
                nameAnalysis(root->children[2],NULL);
            }
            break;

        case SM_ExtDecList_V:
            nameAnalysis(root->children[0],args);
            break;

        case SM_ExtDecList_VCE:
            nameAnalysis(root->children[0],args);
            //child 1 is comma, skip it
            nameAnalysis(root->children[2],args);
            break;

        case SM_Specifiers_T:
            switch(root->children[0]->type_val){
                case RD_INT:
                    *(Sym*)args=sym_int;
                    break;
                case RD_FLOAT:
                    *(Sym*)args=sym_float;
                    break;
                default:
                    //should not reach here
                    printf("Name Analysis: SW_Specifiers_T error!\n");
                    break;
            }
            break;

        case SM_Specifiers_S:
            //production: StructSpecifier -> STRUCT OptTag LC DefList RC
            if(root->children[1]==NULL){
                //anonymous struct here, skip the duplicate check
                
            }

        default:
            printf("name Analysis : AST node error type!\n");
            break;
    }
}