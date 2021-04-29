#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "semantic.h"

extern SymTable global;
extern int hasError;

Sym sym_int, sym_float, global_cur_sym;
char tmpStr[100];

#ifdef DEBUG
int indent = 0;

void printIdent()
{
    for (int i = 0; i < indent; i++)
    {
        printf(" ");
    }
}
#endif

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

FuncParam makeFuncParam(Sym sym)
{
    FuncParam ret = (FuncParam)malloc(sizeof(struct FuncParam_));
    ret->next = NULL;
    ret->type = sym->u.type_sym;
    ret->param = sym;
}

FuncType makeFuncType()
{
    FuncType ret = (FuncType)malloc(sizeof(struct FuncType_));
    ret->n_param = 0;
    ret->head = NULL;
    ret->return_type = NULL;
}

Sym makeSym(enum SYM_ENUM kind, char *name)
{
    Sym ret = (Sym)malloc(sizeof(struct Sym_));
    if (name)
    {
        ret->name = (char *)malloc(strlen(name));
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
        //for VARIABLE or ARRAY, the analysier should link it to a type sim
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

void symTable_addSymTable()
{
    SymTable newST = makeSymTable();
    global->next = newST;
    newST->pre = global;
    global = newST;
}

//We need RAII or GC! Memory leak here.
void symTable_removeTable()
{
    global = global->pre;
    global->next = NULL;
}

Sym symTable_tableFind(SymTable table, char *name, enum SYM_ENUM kind)
{
    Sym cur = table->head;
    if (kind == RD_TYPE || kind == RD_VARIABLE || kind == RD_ARRAY)
    {
        while (cur)
        {
            if (cur->kind == RD_VARIABLE || cur->kind == RD_ARRAY || cur->kind == RD_TYPE)
            {
                if (strcmp(cur->name, name) == 0)
                    return cur;
            }
            cur = cur->next;
        }
    }
    else
    {
        while (cur)
        {
            if (cur->kind == RD_FUNC)
            {
                if (strcmp(cur->name, name) == 0)
                    return cur;
            }
            cur = cur->next;
        }
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
    if (cur && symTable_tableFind(cur, name, kind))
    {
        return 1;
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

#ifdef DEBUG
    indent += 2;
    printIdent();
    printf("##%s##\n", root->name);
#endif

    int ret = 0;
    switch (root->type)
    {
    case SM_Program:
    {
        sym_int = makeSym(RD_TYPE, "int");
        fillBasicType(sym_int->u.type, RD_INT);

        sym_float = makeSym(RD_TYPE, "float");
        fillBasicType(sym_int->u.type, RD_FLOAT);

        global = makeSymTable();
        ret = nameAnalysis(root->children[0], NULL);
    }
    break;

    case SM_ExtDefList:
    {
        ret = nameAnalysis(root->children[0], NULL);
        ret = nameAnalysis(root->children[1], NULL) || ret;
    }
    break;

    case SM_ExtDef_SES:
    {
        //we need sym of the specifier
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        ret = nameAnalysis(root->children[1], (void *)specifier) || ret;
    }
    break;

    case SM_ExtDef_SS:
    {
        ret = nameAnalysis(root->children[0], NULL);
    }
    break;

    case SM_ExtDef_SFC:
    {
        //we need sym of the specifier
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        ret = nameAnalysis(root->children[1], (void *)specifier) || ret;
        ret = nameAnalysis(root->children[2], NULL) || ret;
        symTable_removeTable();
    }
    break;

    case SM_ExtDecList_V:
    {
        ret = nameAnalysis(root->children[0], args);
    }
    break;

    case SM_ExtDecList_VCE:
    {
        ret = nameAnalysis(root->children[0], args);
        ret = nameAnalysis(root->children[2], args) || ret;
    }
    break;

    case SM_Specifiers_T:
    {
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
            printf("Name Analysis: SW_Specifiers_T type error!\n");
            ret = 1;
            break;
        }
    }
    break;

    case SM_Specifiers_S:
    {
        ret = nameAnalysis(root->children[0], args);
    }
    break;

    case SM_StructSpecifier_SOLDR:
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
                ret = 1;
            }
            else
            {
                cur = makeSym(RD_TYPE, root->children[1]->children[0]->str_val);
            }
        }
        if (!ret)
            cur->u.type->kind = RD_STRUCTURE;
        ret = nameAnalysis(root->children[3], (void *)cur);
        if (!ret)
        {
            if (args != NULL)
                *(Sym *)args = cur;
            symTable_addSym(cur);
        }
    }
    break;

    case SM_StructSpecifier_ST:
    {
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
    }
    break;

    case SM_VarDec_I:
    {
        char *name = root->children[0]->str_val;
        int err = symTable_checkDuplicate(name, RD_VARIABLE);
        if (err)
        {
            sprintf(tmpStr, "Redefined variable \"%s\"", name);
            semanticError(3, root->children[0]->lineno, tmpStr);
            ret = 1;
            break;
        }
        Sym cur = makeSym(RD_VARIABLE, name);
        cur->u.type_sym = (Sym)args;
        symTable_addSym(cur);
        global_cur_sym = cur;
    }
    break;

    case SM_VarDec_VLIR:
    {
        ret = nameAnalysis(root->children[0], args);
        if (ret)
        {
            break;
        }
        if (global_cur_sym->kind == RD_VARIABLE)
        {
            global_cur_sym->kind = RD_ARRAY;
            global_cur_sym->u.array_ty.type_sym = global_cur_sym->u.type_sym;
            global_cur_sym->u.array_ty.size = 4 * root->children[2]->int_val;
        }
        else
        {
            global_cur_sym->u.array_ty.size *= root->children[2]->int_val;
        }
    }
    break;

    case SM_FunDec_ILVR:
    {
        char *name = root->children[0]->str_val;
        int err = symTable_checkDuplicate(name, RD_FUNC);
        if (err)
        {
            sprintf(tmpStr, "Redefined function \"%s\"", name);
            semanticError(4, root->children[0]->lineno, tmpStr);
            ret = 1;
        }
        Sym cur = makeSym(RD_FUNC, name);
        cur->u.func_type->return_type = (Sym)args;
        symTable_addSymTable();
        ret = nameAnalysis(root->children[2], cur);
        if (!ret)
            symTable_addSym(cur);
    }
    break;

    case SM_FunDec_ILR:
    {
        char *name = root->children[0]->str_val;
        int err = symTable_checkDuplicate(name, RD_FUNC);
        if (err)
        {
            sprintf(tmpStr, "Redefined function \"%s\"", name);
            semanticError(4, root->children[0]->lineno, tmpStr);
            ret = 1;
        }
        if (!ret)
        {
            Sym cur = makeSym(RD_FUNC, name);
            cur->u.func_type->return_type = (Sym)args;
            symTable_addSym(cur);
        }
        symTable_addSymTable();
    }
    break;

    case SM_VarList_PCV:
    {
        ret = nameAnalysis(root->children[0], args);
        ret = nameAnalysis(root->children[2], args) || ret;
    }
    break;

    case SM_VarList_P:
    {
        ret = nameAnalysis(root->children[0], args);
    }
    break;

    case SM_ParamDec:
    {
        Sym func_sym = (Sym)args;
        //we need sym of the specifier
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        ret = nameAnalysis(root->children[1], (void *)specifier) || ret;
        if (!ret)
        {
            FuncType func_type = func_sym->u.func_type;
            func_type->n_param++;

            FuncParam param = makeFuncParam(global_cur_sym);
            if (!func_type->head)
            {
                func_type->head = param;
            }
            else
            {
                FuncParam cur = func_type->head;
                while (cur->next)
                    cur = cur->next;
                cur->next = param;
            }
        }
    }
    break;

        //To do

    default:
    {
        printf("name Analysis : unimplemented error!\n");
        ret = 1;
    }
    break;
    }

#ifdef DEBUG
    printIdent();
    printf("!%s!\n", root->name);
    indent -= 2;
#endif

    return ret;
}