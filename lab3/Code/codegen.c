#include "stdlib.h"
#include "assert.h"

#include "semantic.h"
#include "codegen.h"

SymTable variables;
extern SymTable globaltype;

Operand zero;
Operand one;

int new_param()
{
    static int cnt = 0;
    return --cnt;
}

int new_tmp()
{
    static int cnt = 0;
    return ++cnt;
}

int new_label()
{
    static int cnt = 0;
    return ++cnt;
}

Operand new_operand(int kind)
{
    Operand ret = (Operand)malloc(sizeof(struct Operand_));
    ret->kind = kind;
}

struct InterCodes *new_codes()
{
    struct InterCodes *ret = (struct InterCodes *)malloc(
        sizeof(struct InterCodes));
    ret->pre = NULL;
    ret->next = NULL;
    ret->end = ret;
}

void concat_codes(struct InterCodes *first, struct InterCodes *second)
{
    if (!first)
    {
        first = second;
        return;
    }
    if (!second)
        return;

    struct InterCodes *first_end = first->end;
    first_end->next = second;
    second->pre = first;
    first->end = second->end;
}

struct InterCodes *gen_func(char *name)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_FUNCNAME;
    ret->code.u.name = name;
    Sym func_sym = symTable_find(globaltype, name, RD_FUNC);
    FuncParam func_param = func_sym->u.func_type->head;
    while (func_param)
    {
        if (func_param->param->kind == RD_VARIABLE)
        {
            struct InterCodes *tmp = new_codes();
            tmp->code.kind = CG_PARAMDEC;
            int param_no = new_param();
            tmp->code.u.var_no = param_no;
            func_param->param->var_no = param_no;
            concat_codes(ret, tmp);
        }
        else
        {
            // to do: add array and struct variabe
            assert(0);
        }
        func_param = func_param->next;
    }
    return ret;
}

struct InterCodes *gen_return(Operand place)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_RETURN;
    ret->code.u.ret = place;
    return ret;
}

struct InterCodes *gen_label(int label_no)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_LABEL;
    ret->code.u.label_no = label_no;
    return ret;
}

struct InterCodes *gen_goto(int label_no)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_GOTO;
    ret->code.u.label_no = label_no;
    return ret;
}

struct InterCodes *gen_assign(Operand left, Operand right)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_ASSIGN;
    ret->code.u.assign.left = left;
    ret->code.u.assign.right = right;
    return ret;
}

struct InterCodes *codeGen(struct ASTNode *root)
{
    symTable_print(globaltype);
    symTable_print(variables);
    zero = new_operand(CG_CONSTANT);
    zero->u.value = 0;
    one = new_operand(CG_CONSTANT);
    one->u.value = 1;
    return codeGen_ExtDefList(root->children[0]);
}

struct InterCodes *codeGen_ExtDefList(struct ASTNode *root)
{
    if (!root)
        return NULL;
    struct InterCodes *first = codeGen_ExtDef(root->children[0]);
    struct InterCodes *second = codeGen_ExtDefList(root->children[1]);
    concat_codes(first, second);
    return first;
}

struct InterCodes *codeGen_ExtDef(struct ASTNode *root)
{
    switch (root->type)
    {
    case SM_ExtDef_SFC:
    {
        struct InterCodes *func_code = gen_func(root->children[1]);
        struct InterCodes *body_code = codeGen_CompSt(root->children[2]);
        concat_codes(func_code, body_code);
        return func_code;
    }
    break;

    default:
        break;
    }
}

struct InterCodes *codeGen_CompSt(struct ASTNode *root)
{
    return codeGen_StmtList(root->children[3]);
}

struct InterCodes *codeGen_StmtList(struct ASTNode *root)
{
    if (!root)
        return NULL;
    struct InterCodes *stmt = codeGen_Stmt(root->children[0]);
    struct InterCodes *stmtlist = codeGen_StmtList(root->children[1]);
    concat_codes(stmt, stmtlist);
}

struct InterCodes *codeGen_Stmt(struct ASTNode *root)
{
    switch (root->type)
    {
    case SM_Stmt_ES:
    {
        return codeGen_Exp(root->children[0], NULL);
    }
    break;

    case SM_Stmt_C:
    {
        return codeGen_CompSt(root->children[0]);
    }
    break;

    case SM_Stmt_RES:
    {
        Operand place = new_operand(CG_VARIABLE);
        place->u.var_no = new_tmp();
        struct InterCodes *exp = codeGen_Exp(root->children[0], place);
        struct InterCodes *ret = gen_return(place);
        concat_codes(exp, ret);
        return exp;
    }
    break;

    case SM_Stmt_ILERS:
    {
        int label1 = new_label();
        int label2 = new_label();
        struct InterCodes *cond = gen_cond(root->children[2], label1, label2);
        struct InterCodes *label1_code = gen_label(label1);
        struct InterCodes *stmt = codeGen_Stmt(root->children[4]);
        struct InterCodes *label2_code = gen_label(label2);
        concat_codes(cond, label1_code);
        concat_codes(cond, stmt);
        concat_codes(cond, label2_code);
        return cond;
    }
    break;

    case SM_Stmt_ILERSES:
    {
        int label1 = new_label();
        int label2 = new_label();
        int label3 = new_label();
        struct InterCodes *cond = gen_cond(root->children[2], label1, label2);
        struct InterCodes *label1_code = gen_label(label1);
        struct InterCodes *stmt1 = codeGen_Stmt(root->children[4]);
        struct InterCodes *goto_label3 = gen_goto(label3);
        struct InterCodes *label2_code = gen_label(label2);
        struct InterCodes *stmt2 = codeGen_Stmt(root->children[6]);
        struct InterCodes *label3_code = gen_label(label3);
        concat_codes(cond, label1_code);
        concat_codes(cond, stmt1);
        concat_codes(cond, goto_label3);
        concat_codes(cond, label2_code);
        concat_codes(cond, stmt2);
        concat_codes(cond, label3_code);
        return cond;
    }
    break;

    case SM_Stmt_WLERS:
    {
        int label1 = new_label();
        int label2 = new_label();
        int label3 = new_label();
        struct InterCodes *label1_code = gen_label(label1);
        struct InterCodes *cond = gen_cond(root->children[2], label2, label3);
        struct InterCodes *label2_code = gen_label(label2);
        struct InterCodes *stmt = codeGen_Stmt(root->children[4]);
        struct InterCodes *goto_label1 = gen_goto(label1);
        struct InterCodes *label3_code = gen_label(label3);
        concat_codes(label1_code, cond);
        concat_codes(label1_code, label2_code);
        concat_codes(label1_code, stmt);
        concat_codes(label1_code, goto_label1);
        concat_codes(label1_code, label3_code);
        return label1_code;
    }
    break;

    default:
        break;
    }
    return NULL;
}

struct InterCodes *codeGen_Exp(struct ASTNode *root, Operand place)
{
    switch (root->type)
    {
    case SM_Exp_INT:
    {
        if (!place)
        {
            return NULL;
        }
        Operand right = new_operand(CG_CONSTANT);
        right->u.value = root->children[0]->int_val;
        return gen_assign(place, right);
    }
    break;

    case SM_Exp_ID:
    {
        if (!place)
        {
            return NULL;
        }
        Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
        if (v->var_no == 0)
            v->var_no = new_tmp();
        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = v->var_no;
        return gen_assign(place, right);
    }
    break;

    case SM_Exp_ASSIGN:
    {
        if (root->children[0]->type = SM_Exp_ID)
        {
            Operand left = new_operand(CG_VARIABLE);
            Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
            if (v->var_no == 0)
                v->var_no = new_tmp();
            left->u.var_no = v->var_no;

            Operand right = new_operand(CG_VARIABLE);
            right->u.var_no = new_tmp();
            struct InterCodes *right_code = codeGen_Exp(root->children[2], right);
            struct InterCodes *assign_code = gen_assign(left, right);
            concat_codes(right_code, assign_code);
            return right_code;
        }
        else
        {
            assert(0);
            //to do!!!
        }
    }
    break;

    case SM_Exp_PLUS:
    case SM_Exp_MINUS:
    case SM_Exp_STAR:
    case SM_Exp_DIV:
    {
        if (!place)
            return NULL;
        Operand left = new_operand(CG_VARIABLE);
        left->u.var_no = new_tmp();
        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *left_codes = codeGen_Exp(root->children[0], left);
        struct InterCodes *right_codes = codeGen_Exp(root->children[2], right);
        int type;
        switch (root->type)
        {
        case SM_Exp_PLUS:
            type = CG_ADD;
            break;
        case SM_Exp_MINUS:
            type = CG_SUB;
            break;
        case SM_Exp_STAR:
            type = CG_MUL;
            break;
        case SM_Exp_DIV:
            type = CG_DIV;
            break;
        default:
            break;
        }
        struct InterCodes *merge_codes = gen_arith(place, left, right, type);
        concat_codes(left_codes, right_codes);
        concat_codes(left_codes, merge_codes);
        return left_codes;
    }
    break;

    case SM_Exp_MINUSE:
    {
        if (!place)
            return NULL;

        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *right_codes = codeGen_Exp(root->children[1], right);
        struct InterCodes *merge_codes = gen_arithm(place, zero, right);

        concat_codes(right_codes, merge_codes);
        return right_codes;
    }
    break;

    case SM_Exp_AND:
    case SM_Exp_OR:
    case SM_Exp_RELOP:
    case SM_Exp_NOTE:
    {
        if (!place)
            return NULL;
        int label1 = new_label();
        int label2 = new_labe2();
        struct InterCodes *zero_codes = gen_assign(place, zero);
        struct InterCodes *cond_codes = gen_cond(root, label1, label2);
        struct InterCodes *label1_codes = gen_label(label1);
        struct InterCodes *one_codes = gen_assign(place, one);
        struct InterCodes *label2_codes = gen_label(label2);
        concat_codes(zero_codes, cond_codes);
        concat_codes(zero_codes, label1_codes);
        concat_codes(zero_codes, one_codes);
        concat_codes(zero_codes, label2_codes);
        return zero_codes;
    }
    break;

    case SM_Exp_LPERP:
    {
        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *right_codes = codeGen_Exp(root->children[1], right);
        struct InterCodes *assign_codes = gen_assign(place, right);
        concat_codes(right_codes, assign_codes);
        return right_codes;
    }
    break;

    default:
        break;
    }
}