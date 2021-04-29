#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "semantic.h"

SymTable global;

Sym sym_int, sym_float, global_curDef_sym, global_curFunc_sym, global_curExp_typeSym;
FuncParam global_funcparam;
int isInStruct = 0;

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
    //one error for a line
    static char a[100000];
    if(!(a[lineno>>3]&(1<<(lineno&0x7)))){    
        a[lineno>>3]|=1<<(lineno&0x7);
        printf("Error type %d at Line %d: %s.\n", errType, lineno, msg);
    }
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

void initStructType(Type type)
{
    type->kind = RD_STRUCTURE;
    type->u.structure = NULL;
}

FieldList makeFieldList(Sym sym)
{
    FieldList ret = (FieldList)malloc(sizeof(struct FieldList_));
    ret->name = sym->name;
    ret->type_sym = sym;
    ret->tail = NULL;
}

void structType_addSym(Type stype, Sym sym)
{
    if (!stype->u.structure)
    {
        stype->u.structure = makeFieldList(sym);
    }
    else
    {
        FieldList cur = stype->u.structure;
        while (cur->tail)
        {
            cur = cur->tail;
        }
        cur->tail = makeFieldList(sym);
    }
}

void structType_addSymTable(Type stype, SymTable table)
{
    Sym cur = table->head;
    while (cur)
    {
        structType_addSym(stype, cur);
        cur = cur->next;
    }
}

Sym structType_findFeild(Sym struct_sym, char*name){
    Type stype=struct_sym->u.type;
    FieldList cur=stype->u.structure;
    while(cur){
        if(strcmp(cur->name,name)==0){
            return cur->type_sym;
        }
        cur=cur->tail;
    }
    return NULL;
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

int typeSym_IsInt(Sym sym){
    return sym->kind==RD_TYPE && sym->u.type->kind==RD_BASIC && sym->u.type->u.basic==RD_INT;
}

int typeSym_IsBasic(Sym sym){
    return sym->kind==RD_TYPE && sym->u.type->kind==RD_BASIC;
}

int typeSym_IsStruct(Sym sym){
    return sym->kind==RD_TYPE && sym->u.type->kind==RD_STRUCTURE;
}

int typeSym_IsSame(Sym left_sym,Sym right_sym){
    if(!left_sym || !right_sym)
        return 0;
    if(left_sym->kind!=RD_TYPE || right_sym->kind!=RD_TYPE)
        return 0;
    if(left_sym->u.type->kind!=right_sym->u.type->kind)
        return 0;
    if(left_sym->u.type->kind==RD_BASIC){
        return left_sym->u.type->u.basic==right_sym->u.type->u.basic;
    }else{
        FieldList left_field=left_sym->u.type->u.structure;
        FieldList right_field=right_sym->u.type->u.structure;
        int ret=1;
        while(1){
            if(left_field==NULL && right_field==NULL)
                break;
            if((left_field==NULL && right_field!=NULL) || (left_field!=NULL && right_field==NULL)){
                ret=0;
                break;
            }
            ret=ret && typeSym_IsSame(left_field->type_sym,right_field->type_sym);
            if(ret==0)
                break;
            left_field=left_field->tail;
            right_field=right_field->tail;
        }
        return ret;
    }

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
            cur = makeSym(RD_TYPE, root->children[1]->children[0]->str_val);
        }
        initStructType(cur->u.type);

        symTable_addSymTable();
        isInStruct++;
        ret = nameAnalysis(root->children[3], (void *)cur);
        isInStruct--;
        //current sym table now stores content for the struct
        structType_addSymTable(cur->u.type, global);

        symTable_removeTable();
        if (args != NULL)
            *(Sym *)args = cur;
        if (!ret)
            symTable_addSym(cur);
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
            if (!isInStruct)
            {
                sprintf(tmpStr, "Redefined variable \"%s\"", name);
            }
            else
            {
                sprintf(tmpStr, "Redefined field \"%s\"", name);
            }
            semanticError(3, root->children[0]->lineno, tmpStr);
            ret = 1;
            break;
        }
        if (!ret)
        {
            Sym cur = makeSym(RD_VARIABLE, name);
            cur->u.type_sym = (Sym)args;
            symTable_addSym(cur);
            global_curDef_sym = cur;
        }
    }
    break;

    case SM_VarDec_VLIR:
    {
        ret = nameAnalysis(root->children[0], args);
        if (ret)
        {
            break;
        }
        if (global_curDef_sym->kind == RD_VARIABLE)
        {
            global_curDef_sym->kind = RD_ARRAY;
            global_curDef_sym->u.array_ty.type_sym = global_curDef_sym->u.type_sym;
            global_curDef_sym->u.array_ty.size = 4 * root->children[2]->int_val;
        }
        else
        {
            global_curDef_sym->u.array_ty.size *= root->children[2]->int_val;
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
        symTable_addSym(cur);
        global_curFunc_sym = cur;
        symTable_addSymTable();
        ret = nameAnalysis(root->children[2], cur) || ret;
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

        Sym cur = makeSym(RD_FUNC, name);
        cur->u.func_type->return_type = (Sym)args;
        symTable_addSym(cur);

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

            FuncParam param = makeFuncParam(global_curDef_sym);
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

    case SM_CompSt:
    {
        ret = nameAnalysis(root->children[1], NULL);
        ret = nameAnalysis(root->children[2], NULL) || ret;
    }
    break;

    case SM_StmtList:
    {
        ret = nameAnalysis(root->children[0], NULL);
        ret = nameAnalysis(root->children[1], NULL) || ret;
    }
    break;

    case SM_Stmt_ES:
    {
        ret = nameAnalysis(root->children[0], NULL);
    }
    break;

    case SM_Stmt_C:
    {
        symTable_addSymTable();
        ret = nameAnalysis(root->children[0], NULL);
        symTable_removeTable();
    }
    break;

    case SM_Stmt_RES:
    {
        ret = nameAnalysis(root->children[1], NULL);
        if (!typeSym_IsSame(global_curExp_typeSym, global_curFunc_sym->u.func_type->return_type))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for return");
            semanticError(8, root->children[0]->lineno, tmpStr);
        }
    }
    break;

    case SM_Stmt_ILERS:
    {
        ret = nameAnalysis(root->children[2], NULL);
        if (global_curExp_typeSym->u.type->kind != RD_BASIC || global_curExp_typeSym->u.type->u.basic != RD_INT)
        {
            ret = 1;
            sprintf(tmpStr, "Exp type mismatched for if, should be INT");
            semanticError(7, root->children[2]->lineno, tmpStr);
        }
        ret = nameAnalysis(root->children[4], NULL) || ret;
    }
    break;

    case SM_Stmt_ILERSES:
    {
        ret = nameAnalysis(root->children[2], NULL);
        if (global_curExp_typeSym->u.type->kind != RD_BASIC || global_curExp_typeSym->u.type->u.basic != RD_INT)
        {
            ret = 1;
            sprintf(tmpStr, "Condition type mismatched for if, should be INT");
            semanticError(7, root->children[2]->lineno, tmpStr);
        }
        ret = nameAnalysis(root->children[4], NULL) || ret;
        ret = nameAnalysis(root->children[6], NULL) || ret;
    }
    break;
        //To do

    case SM_Stmt_WLERS:
    {
        ret = nameAnalysis(root->children[2], NULL);
        if (global_curExp_typeSym->u.type->kind != RD_BASIC || global_curExp_typeSym->u.type->u.basic != RD_INT)
        {
            ret = 1;
            sprintf(tmpStr, "Condition type mismatched for while, should be INT");
            semanticError(7, root->children[2]->lineno, tmpStr);
        }
        ret = nameAnalysis(root->children[4], NULL) || ret;
    }
    break;

    case SM_DefList:
    {
        ret = nameAnalysis(root->children[0], NULL);
        ret = nameAnalysis(root->children[1], NULL) || ret;
    }
    break;

    case SM_Def:
    {
        Sym specifier = NULL;
        ret = nameAnalysis(root->children[0], (void *)&specifier);
        ret = nameAnalysis(root->children[1], (void *)specifier) || ret;
    }
    break;

    case SM_DecList_D:
    {
        ret = nameAnalysis(root->children[0], args);
    }
    break;

    case SM_DecList_DCD:
    {
        ret = nameAnalysis(root->children[0], args);
        ret = nameAnalysis(root->children[2], args) || ret;
    }
    break;

    case SM_Dec_V:
    {
        ret = nameAnalysis(root->children[0], args);
    }
    break;

    case SM_Dec_VAE:
    {
        ret = nameAnalysis(root->children[0], args);
        if (isInStruct)
        {
            ret = 1;
            sprintf(tmpStr, "Invalid assignment in struct define");
            semanticError(15, root->children[0]->lineno, tmpStr);
            break;
        }
        ret = nameAnalysis(root->children[2], NULL);
        if (!typeSym_IsSame((Sym)args, global_curExp_typeSym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for operands");
            semanticError(7, root->children[1]->lineno, tmpStr);
            break;
        }
    }
    break;

    case SM_Exp_ASSIGN:
    {
        enum NodeType nodeType = root->children[0]->type;
        //check if left side is left value
        if (nodeType != SM_Exp_ID && nodeType != SM_Exp_ELBERB && nodeType != SM_Exp_EDI)
        {
            ret = 1;
            sprintf(tmpStr, "The left-hand side of an assignment must be a variable");
            semanticError(6, root->children[0]->lineno, tmpStr);
            ret = nameAnalysis(root->children[2], NULL) || ret;
            global_curExp_typeSym = NULL;
            break;
        }
        ret = nameAnalysis(root->children[0], NULL);
        Sym left_sym = global_curExp_typeSym;
        ret = nameAnalysis(root->children[2], NULL) || ret;
        Sym right_sym = global_curExp_typeSym;
        if (!typeSym_IsSame(left_sym, right_sym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for assignment");
            semanticError(5, root->children[0]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        global_curExp_typeSym = left_sym;
    }
    break;

    case SM_Exp_AND:
    case SM_Exp_OR:
    {
        ret = nameAnalysis(root->children[0], NULL);
        Sym left_sym = global_curExp_typeSym;
        ret = nameAnalysis(root->children[2], NULL) || ret;
        Sym right_sym = global_curExp_typeSym;
        if (!typeSym_IsInt(left_sym) || !typeSym_IsInt(right_sym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for operands");
            semanticError(7, root->children[1]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        global_curExp_typeSym = left_sym;
    }
    break;

    case SM_Exp_RELOP:
    case SM_Exp_PLUS:
    case SM_Exp_MINUS:
    case SM_Exp_STAR:
    case SM_Exp_DIV:
    {
        ret = nameAnalysis(root->children[0], NULL);
        Sym left_sym = global_curExp_typeSym;
        ret = nameAnalysis(root->children[2], NULL) || ret;
        Sym right_sym = global_curExp_typeSym;
        if (!typeSym_IsBasic(left_sym) || !typeSym_IsBasic(right_sym) ||
            !typeSym_IsSame(left_sym, right_sym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for operands");
            semanticError(7, root->children[1]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }

        if (root->type == SM_Exp_RELOP)
            global_curExp_typeSym = sym_int;
        else
            global_curExp_typeSym = left_sym;
    }
    break;

    case SM_Exp_LPERP:
    {
        ret = nameAnalysis(root->children[1], NULL);
    }
    break;

    case SM_Exp_MINUSE:
    {
        ret = nameAnalysis(root->children[1], NULL);
        Sym sym = global_curExp_typeSym;
        if (!typeSym_IsBasic(sym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for operands");
            semanticError(7, root->children[1]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
    }
    break;

    case SM_Exp_NOTE:
    {
        ret = nameAnalysis(root->children[1], NULL);
        Sym sym = global_curExp_typeSym;
        if (!typeSym_IsInt(sym))
        {
            ret = 1;
            sprintf(tmpStr, "Type mismatched for operands");
            semanticError(7, root->children[1]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
    }
    break;

    case SM_Exp_ILPARP:
    {
        char *name = root->children[0]->str_val;
        Sym cur_func = symTable_find(name, RD_FUNC);
        if (cur_func == NULL)
        {
            ret = 1;
            Sym err11 = symTable_find(name, RD_VARIABLE);
            if (err11)
            {
                sprintf(tmpStr, "\"%s\" is not a function", name);
                semanticError(2, root->children[0]->lineno, tmpStr);
            }
            else
            {
                sprintf(tmpStr, "Undefined function \"%s\"", name);
                semanticError(2, root->children[0]->lineno, tmpStr);
            }
            global_curExp_typeSym = NULL;
            break;
        }
        global_funcparam = NULL;
        ret = nameAnalysis(root->children[2], cur_func);
        if (ret)
        {
            sprintf(tmpStr, "Function is not applicable for arguments");
            semanticError(9, root->children[2]->lineno, tmpStr);
        }
        global_curExp_typeSym = cur_func->u.func_type->return_type;
    }

    case SM_Exp_ILPRP:
    {
        char *name = root->children[0]->str_val;
        Sym cur_func = symTable_find(name, RD_FUNC);
        if (cur_func == NULL)
        {
            ret = 1;
            Sym err11 = symTable_find(name, RD_VARIABLE);
            if (err11)
            {
                sprintf(tmpStr, "\"%s\" is not a function", name);
                semanticError(2, root->children[0]->lineno, tmpStr);
            }
            else
            {
                sprintf(tmpStr, "Undefined function \"%s\"", name);
                semanticError(2, root->children[0]->lineno, tmpStr);
            }
            global_curExp_typeSym = NULL;
            break;
        }
        if (cur_func->u.func_type->n_param > 0)
        {
            ret = 1;
            sprintf(tmpStr, "Function is not applicable for arguments");
            semanticError(9, root->children[2]->lineno, tmpStr);
        }
        global_curExp_typeSym = cur_func->u.func_type->return_type;
    }
    break;

    case SM_Exp_ELBERB:
    {
        ret = nameAnalysis(root->children[0], NULL);
        if (ret)
        {
            global_curExp_typeSym = NULL;
            break;
        }
        Sym array_sym = global_curExp_typeSym;
        if (array_sym->kind != RD_ARRAY)
        {
            ret = 1;
            sprintf(tmpStr, "\"%s\" is not an array", root->children[0]->str_val);
            semanticError(10, root->children[2]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        ret = nameAnalysis(root->children[2], NULL);
        if (ret)
        {
            global_curExp_typeSym = NULL;
            break;
        }
        Sym int_sym = global_curExp_typeSym;
        if (!typeSym_IsInt(int_sym))
        {
            ret = 1;
            sprintf(tmpStr, "\"%s\" is not an integer", root->children[2]->str_val);
            semanticError(12, root->children[2]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        global_curExp_typeSym = array_sym->u.array_ty.type_sym;
    }
    break;

    case SM_Exp_EDI:
    {
        ret = nameAnalysis(root->children[0], NULL);
        if (ret)
        {
            global_curExp_typeSym = NULL;
            break;
        }
        Sym struct_sym = global_curExp_typeSym;
        if (!typeSym_IsStruct(struct_sym))
        {
            ret = 1;
            sprintf(tmpStr, "Illegal use of \".\"");
            semanticError(13, root->children[2]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }

        char *name = root->children[2]->str_val;
        Sym field_sym = structType_findFeild(struct_sym,name);
        if (field_sym == NULL)
        {
            ret = 1;
            sprintf(tmpStr, "Non-existent field \"%s\"", name);
            semanticError(14, root->children[2]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        global_curExp_typeSym = field_sym->u.type_sym;
    }
    break;

    case SM_Exp_ID:
    {
        char *name = root->children[0]->str_val;
        Sym vari_sym = symTable_find(name, RD_VARIABLE);
        if (vari_sym == NULL)
        {
            ret = 1;
            sprintf(tmpStr, "Undefined variable \"%s\"", name);
            semanticError(1, root->children[0]->lineno, tmpStr);
            global_curExp_typeSym = NULL;
            break;
        }
        global_curExp_typeSym = vari_sym->u.type_sym;
    }
    break;

    case SM_Exp_INT:
    {
        global_curExp_typeSym = sym_int;
    }
    break;

    case SM_Exp_FLOAT:
    {
        global_curExp_typeSym = sym_float;
    }
    break;

    case SM_Args_ECA:
    {
        ret = nameAnalysis(root->children[0], NULL);
        if (ret)
            break;
        Sym left_sym = global_curExp_typeSym;
        Sym cur = NULL;
        if (global_funcparam == NULL)
        {
            Sym func_sym = (Sym)args;
            if (func_sym->u.func_type->head == NULL)
            {
                ret = 1;
                break;
            }
            else
            {
                global_funcparam = func_sym->u.func_type->head;
                cur = global_funcparam->type;
            }
        }
        else
        {
            if (global_funcparam->next == NULL)
            {
                ret = 1;
                break;
            }
            else
            {
                global_funcparam = global_funcparam->next;
                cur = global_funcparam->type;
            }
        }
        if (!typeSym_IsSame(left_sym, cur))
        {
            ret = 1;
            break;
        }
        ret = nameAnalysis(root->children[2], NULL) || ret;
    }

    case SM_Args_E:
    {
        ret = nameAnalysis(root->children[0], NULL);
        if (ret)
            break;
        Sym left_sym = global_curExp_typeSym;
        Sym cur = NULL;
        if (global_funcparam == NULL)
        {
            Sym func_sym = (Sym)args;
            if (func_sym->u.func_type->head == NULL)
            {
                ret = 1;
                break;
            }
            else
            {
                global_funcparam = func_sym->u.func_type->head;
                cur = global_funcparam->type;
            }
        }
        else
        {
            if (global_funcparam->next == NULL)
            {
                ret = 1;
                break;
            }
            else
            {
                global_funcparam = global_funcparam->next;
                cur = global_funcparam->type;
            }
        }
        if (!typeSym_IsSame(left_sym, cur))
        {
            ret = 1;
            break;
        }
        if (global_funcparam->next)
        {
            ret = 1;
            break;
        }
    }
    break;

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