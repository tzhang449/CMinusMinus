#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "semantic.h"

extern SymTable global;
extern int hasError;

Sym sym_int, sym_float;
char tmpStr[100];

void semanticError(int errType, int lineno, char *msg)
{
    printf("Error type %d at Line %d: %s.\n", errType, lineno, msg);
}

Type makeType()
{
    return (Type)malloc(sizeof(struct Type_));
}

void fillBasicType(Type type, enum TYPE_ENUM basic)
{
    type->kind = RD_BASIC;
    type->u.basic = basic;
}

FuncType makeFuncType()
{
    return (FuncType)malloc(sizeof(struct FuncType_));
}

Sym makeSym(enum SYM_ENUM kind, char *name)
{
    Sym ret = (Sym)malloc(sizeof(struct Sym_));
    if (name)
    {
        strcpy(ret->name, name);
    }
    else
    {
        ret->name = NULL;
    }

    ret->kind = kind;

    switch (ret->kind)
    {
    case RD_TYPE:
        ret->u.type = makeType();
        break;
    case RD_FUNC:
        ret->u.func_type = makeFuncType();
        break;
    default:
        ret->u.type_sym = NULL;
        //for VARIABLE, the analysier should link it to a type sim
        break;
    }

    ret->node = NULL;
    ret->next = NULL;
    return ret;
}

SymTable makeSymTable()
{
    SymTable ret = (SymTable)malloc(sizeof(struct SymTable_));
    ret->head = NULL;
    ret->next = NULL;
    ret->pre = NULL;
    return ret;
}

void symTable_addSym(Sym sym)
{
    if (!global->head)
    {
        global->head = sym;
        return;
    }

    Sym cur = global->head;
    while (cur->next)
        cur = cur->next;
    cur->next = sym;
}

Sym symTable_tableFind(SymTable table, char *name, enum SYM_ENUM kind)
{
    Sym cur = table->head;
    while (cur)
    {
        if ((cur->kind == RD_TYPE || cur->kind == RD_VARIABLE) && (kind == RD_TYPE || kind == RD_VARIABLE))
        {
            if (strcmp(cur->name, name) == 0)
                return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

Sym symTable_find(char *name, enum SYM_ENUM kind)
{
    SymTable cur = global;
    Sym ret = NULL;
    while (cur)
    {
        ret = symTable_tableFind(cur, name, kind);
        if (ret)
        {
            return ret;
        }
        cur = cur->pre;
    }
    return ret;
}

int symTable_checkDuplicate(char *name, enum SYM_ENUM kind)
{
    SymTable cur = global;
    while (cur)
    {
        if (symTable_tableFind(cur, name, kind))
        {
            return 1;
        }
        cur = cur->pre;
    }
    return 0;
}

// on success, 0 is returned. Otherwise, 1 is returned.
int nameAnalysis(struct ASTNode *root, void *args)
{
    if (root == NULL)
    {
        return 0;
    }
    int ret = 0;
    switch (root->type)
    {
    case SM_Program:
        sym_int = makeSym(RD_TYPE, "int");
        fillBasicType(sym_int->u.type, RD_INT);

        sym_float = makeSym(RD_TYPE, "float");
        fillBasicType(sym_int->u.type, RD_FLOAT);

        global = makeSymTable();
        ret = nameAnalysis(root->children[0], NULL);
        break;

    case SM_ExtDefList:
        ret = nameAnalysis(root->children[0], NULL);
        ret = ret || nameAnalysis(root->children[1], NULL);
        break;

    case SM_ExtDef_SES:
    {
        //we need sym of the specifier
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        if (!ret)
            nameAnalysis(root->children[1], (void *)specifier);
    }
    break;

    case SM_ExtDef_SS:
        ret = nameAnalysis(root->children[0], NULL);
        break;

    case SM_ExtDef_SFC:
    {
        //we need sym of the specifier
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        if (!ret)
            ret = nameAnalysis(root->children[1], (void *)specifier);
        if (!ret)
            ret = nameAnalysis(root->children[2], NULL);
    }
    break;

    case SM_ExtDecList_V:
        ret = nameAnalysis(root->children[0], args);
        break;

    case SM_ExtDecList_VCE:
        ret = nameAnalysis(root->children[0], args);
        //child 1 is comma, skip it
        ret = ret || nameAnalysis(root->children[2], args);
        break;

    case SM_Specifiers_T:
        switch (root->children[0]->type_val)
        {
        case RD_INT:
            *(Sym *)args = sym_int;
            break;
        case RD_FLOAT:
            *(Sym *)args = sym_float;
            break;
        default:
            //should not reach here
            printf("Name Analysis: SW_Specifiers_T error!\n");
            ret = 1;
            break;
        }
        break;

    case SM_Specifiers_S:
        ret = nameAnalysis(root->children[0], args);
        break;

    case SM_StructSpecifier_SOLDR:
        //production: StructSpecifier -> STRUCT OptTag LC DefList RC
        {
            Sym cur = NULL;
            if (root->children[1] == NULL)
            {
                //anonymous struct, skip the duplicate check
                cur = makeSym(RD_TYPE, NULL);
            }
            else
            {
                char *name = root->children[1]->children[0]->str_val;

                int err = symTable_checkDuplicate(name, RD_TYPE);
                if (err)
                {
                    sprintf(tmpStr, "Duplicated name \"%s\"", name);
                    semanticError(16, root->children[1]->lineno, tmpStr);
                    *(Sym *)args = NULL;
                    return 1;
                }

                cur = makeSym(RD_TYPE, root->children[1]->children[0]->str_val);
            }
            cur->u.type->kind = RD_STRUCTURE;
            nameAnalysis(root->children[3], (void *)&cur);
            if (!ret)
            {
                *(Sym *)args = cur;
                symTable_addSym(cur);
            }
            else
            {
                *(Sym *)args = NULL;
            }
        }
        break;

    case SM_StructSpecifier_ST:
        char *name = root->children[1]->children[0]->str_val;
        Sym cur = symTable_find(name, RD_TYPE);
        if (!cur)
        {
            ret = 1;
            sprintf(tmpStr, "Undefined structure \"%s\"", name);
            semanticError(17, root->children[1]->lineno, tmpStr);
        }
        else
        {
            *(Sym *)args = cur;
        }
        break;



        //To do

    default:
        printf("name Analysis : unimplemented error!\n");
        ret = 1;
        break;
    }

    return ret;
}