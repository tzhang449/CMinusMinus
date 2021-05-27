#include "stdlib.h"
#include "assert.h"
#include "math.h"
#include "string.h"

#include "semantic.h"
#include "codegen.h"

SymTable variables;
extern SymTable globaltype;

Operand zero;
Operand one;

void print_operand(Operand v, FILE *file)
{
    switch (v->kind)
    {
    case CG_VARIABLE:
        fprintf(file, "%s%d", v->u.var_no > 0 ? "t" : "v", abs(v->u.var_no));
        break;

    case CG_CONSTANT:
        fprintf(file, "#%d", v->u.value);
        break;

    //to do
    default:
        assert(0);
        break;
    }
}

void print_code(struct InterCode code, char *str, FILE *file)
{
    switch (code.kind)
    {
    case CG_FUNCNAME:
        fprintf(file, "FUNCTION %s :", code.u.name);
        break;
    case CG_PARAMDEC:
        fprintf(file, "PARAM v%d ", abs(code.u.var_no));
        break;
    case CG_RETURN:
        fprintf(file, "RETURN ");
        print_operand(code.u.ret, file);
        break;
    case CG_LABEL:
        fprintf(file, "LABEL label%d:", code.u.label_no);
        break;
    case CG_GOTO:
        fprintf(file, "GOTO label%d", code.u.label_no);
        break;
    case CG_RELOPGOTO:
        //IF x [relop] y GOTO z
        fprintf(file, "IF ");
        print_operand(code.u.relopgoto.left, file);
        fprintf(file, " %s ", code.u.relopgoto.op);
        print_operand(code.u.relopgoto.right, file);
        fprintf(file, " GOTO label%d", code.u.relopgoto.label_no);
        break;
    case CG_ASSIGN:
        print_operand(code.u.assign.left, file);
        fprintf(file, " := ");
        print_operand(code.u.assign.right, file);
        break;
    case CG_ADD:
    case CG_SUB:
    case CG_MUL:
    case CG_DIV:
        print_operand(code.u.binop.result, file);
        fprintf(file, " := ");
        print_operand(code.u.binop.op1, file);
        char *sign;
        switch (code.kind)
        {
        case CG_ADD:
            sign = "+";
            break;
        case CG_SUB:
            sign = "-";
            break;
        case CG_MUL:
            sign = "*";
            break;
        case CG_DIV:
            sign = "/";
            break;
        default:
            break;
        }
        fprintf(file, " %s ", sign);
        print_operand(code.u.binop.op2, file);
        break;

    case CG_READ:
        fprintf(file, "READ ");
        print_operand(code.u.func_call.place, file);
        break;

    case CG_CALL:
        print_operand(code.u.func_call.place, file);
        fprintf(file, " := CALL %s", code.u.func_call.name);
        break;

    //to do
    default:
        assert(0);
        break;
    }
    fprintf(file, "%s", str);
}

void print_codes(struct InterCodes *codes, FILE *file)
{
    struct InterCodes *head = codes;
    while (head)
    {
        print_code(head->code, "\n", file);
        head = head->next;
    }
}

int new_param()
{
    static int param_cnt = 0;
    return --param_cnt;
}

int tmp_cnt = 0;
int tmp_map[10000];

int new_tmp()
{
    for (int i = 0; i < 10000; i++)
    {
        if ((~tmp_map[i]) == 0)
            continue;
        int j = __builtin_popcount(tmp_map[i] ^ (tmp_map[i] + 1));
        tmp_map[i] |= 1 << (j - 1);
        return i * 32 + j;
    }
    tmp_cnt++;
    return tmp_cnt + 10000 * 32;
}

void destroy_tmp(int i)
{
    if (i > 10000 * 32)
        return;
    i = i - 1;
    tmp_map[i >> 5] &= ~(1 << (i % 32));
}

int new_label()
{
    static int label_cnt = 0;
    return ++label_cnt;
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

struct InterCodes *concat_codes(struct InterCodes *first, struct InterCodes *second)
{
    if (!first)
    {
        first = second;
    }
    else if (second)
    {

        struct InterCodes *first_end = first->end;
        first_end->next = second;
        second->pre = first;
        first->end = second->end;
    }
    return first;
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

            Sym sym = symTable_find(variables, func_param->param->name, RD_VARIABLE);
            sym->var_no = param_no;

            ret = concat_codes(ret, tmp);
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

struct InterCodes *gen_arith(Operand place, Operand left, Operand right, int type)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = type;
    ret->code.u.binop.result = place;
    ret->code.u.binop.op1 = left;
    ret->code.u.binop.op2 = right;
    return ret;
}

struct InterCodes *gen_relopGoto(Operand left, Operand right, char *op, int label)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_RELOPGOTO;
    ret->code.u.relopgoto.left = left;
    ret->code.u.relopgoto.right = right;
    ret->code.u.relopgoto.op = op;
    ret->code.u.relopgoto.label_no = label;
    return ret;
}

struct InterCodes *gen_cond(struct ASTNode *root, int label_true, int label_false)
{
    switch (root->type)
    {
    case SM_Exp_RELOP:
    {
        Operand left = new_operand(CG_VARIABLE);
        left->u.var_no = new_tmp();
        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *left_codes = codeGen_Exp(root->children[0], left);
        struct InterCodes *right_codes = codeGen_Exp(root->children[2], right);
        struct InterCodes *relopGoto_codes = gen_relopGoto(left, right, root->children[1]->str_val, label_true);
        struct InterCodes *goto_codes = gen_goto(label_false);
        left_codes = concat_codes(left_codes, right_codes);
        left_codes = concat_codes(left_codes, relopGoto_codes);
        left_codes = concat_codes(left_codes, goto_codes);
        return left_codes;
    }
    break;
    case SM_Exp_AND:
    {
        int label1 = new_label();
        struct InterCodes *left_codes = gen_cond(root->children[0], label1, label_false);
        struct InterCodes *label1_codes = gen_label(label1);
        struct InterCodes *right_codes = gen_cond(root->children[2], label_true, label_false);
        left_codes = concat_codes(left_codes, label1_codes);
        left_codes = concat_codes(left_codes, right_codes);
        return left_codes;
    }
    break;
    case SM_Exp_OR:
    {
        int label1 = new_label();
        struct InterCodes *left_codes = gen_cond(root->children[0], label_true, label1);
        struct InterCodes *label1_codes = gen_label(label1);
        struct InterCodes *right_codes = gen_cond(root->children[2], label_true, label_false);
        left_codes = concat_codes(left_codes, label1_codes);
        left_codes = concat_codes(left_codes, right_codes);
        return left_codes;
    }
    break;
    case SM_Exp_NOTE:
    {
        return gen_cond(root->children[1], label_false, label_true);
    }
    break;
    default:
    {
        //other cases
        Operand place = new_operand(CG_VARIABLE);
        place->u.var_no = new_tmp();
        struct InterCodes *exp_codes = codeGen_Exp(root, place);
        struct InterCodes *relopGoto_codes = gen_relopGoto(place, zero, "!=", label_true);
        struct InterCodes *goto_codes = gen_goto(label_false);
        exp_codes = concat_codes(exp_codes, relopGoto_codes);
        exp_codes = concat_codes(exp_codes, goto_codes);
        return exp_codes;
    }
    break;
    }
}

struct InterCodes *gen_read(Operand place)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_READ;
    if (place == NULL)
    {
        place = new_operand(CG_VARIABLE);
        place->u.var_no = new_tmp();
        destroy_tmp(place->u.var_no);
    }
    ret->code.u.func_call.place = place;
}

struct InterCodes *gen_call(Operand place, char *name)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_CALL;
    ret->code.u.func_call.name = name;
    if (place == NULL)
    {
        place = new_operand(CG_VARIABLE);
        place->u.var_no = new_tmp();
        destroy_tmp(place->u.var_no);
    }
    ret->code.u.func_call.place = place;
}

struct InterCodes *codeGen(struct ASTNode *root)
{
    printf("############################\n");
    symTable_print(globaltype);
    printf("############################\n");
    symTable_print(variables);
    printf("############################\n");
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
    first = concat_codes(first, second);
    return first;
}

struct InterCodes *codeGen_ExtDef(struct ASTNode *root)
{
    switch (root->type)
    {
    case SM_ExtDef_SFC:
    {
        struct InterCodes *func_code = gen_func(root->children[1]->children[0]->str_val);
        struct InterCodes *body_code = codeGen_CompSt(root->children[2]);
        func_code = concat_codes(func_code, body_code);
        return func_code;
    }
    break;

    default:
        break;
    }
}

struct InterCodes *codeGen_CompSt(struct ASTNode *root)
{
    return codeGen_StmtList(root->children[2]);
}

struct InterCodes *codeGen_StmtList(struct ASTNode *root)
{
    if (!root)
        return NULL;
    struct InterCodes *stmt = codeGen_Stmt(root->children[0]);
    struct InterCodes *stmtlist = codeGen_StmtList(root->children[1]);
    stmt = concat_codes(stmt, stmtlist);
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
        struct InterCodes *exp = codeGen_Exp(root->children[1], place);
        struct InterCodes *ret = gen_return(place);
        exp = concat_codes(exp, ret);
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
        cond = concat_codes(cond, label1_code);
        cond = concat_codes(cond, stmt);
        cond = concat_codes(cond, label2_code);
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
        cond = concat_codes(cond, label1_code);
        cond = concat_codes(cond, stmt1);
        cond = concat_codes(cond, goto_label3);
        cond = concat_codes(cond, label2_code);
        cond = concat_codes(cond, stmt2);
        cond = concat_codes(cond, label3_code);
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
        label1_code = concat_codes(label1_code, cond);
        label1_code = concat_codes(label1_code, label2_code);
        label1_code = concat_codes(label1_code, stmt);
        label1_code = concat_codes(label1_code, goto_label1);
        label1_code = concat_codes(label1_code, label3_code);
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
        if (place->kind != CG_VARIABLE)
        {
            Operand right = new_operand(CG_CONSTANT);
            right->u.value = root->children[0]->int_val;
            return gen_assign(place, right);
        }
        else
        {
            destroy_tmp(place->u.var_no);
            place->kind = CG_CONSTANT;
            place->u.value = root->children[0]->int_val;
            return NULL;
        }
    }
    break;

    case SM_Exp_ID:
    {
        if (!place)
        {
            return NULL;
        }

        if (place->kind != CG_VARIABLE)
        {
            Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
            if (v->var_no == 0)
            {
                v->var_no = new_tmp();
            }
            Operand right = new_operand(CG_VARIABLE);
            right->u.var_no = v->var_no;
            return gen_assign(place, right);
        }
        else
        {
            destroy_tmp(place->u.var_no);
            Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
            if (v->var_no == 0)
            {
                v->var_no = new_tmp();
            }
            place->u.var_no = v->var_no;
            return NULL;
        }
    }
    break;

    case SM_Exp_ASSIGN:
    {
        if (root->children[0]->type = SM_Exp_ID)
        {
            Operand left = new_operand(CG_VARIABLE);
            Sym v = symTable_find(variables, root->children[0]->children[0]->str_val, RD_VARIABLE);
            if (v->var_no == 0)
                v->var_no = new_tmp();
            left->u.var_no = v->var_no;

            Operand right = new_operand(CG_VARIABLE);
            right->u.var_no = new_tmp();
            struct InterCodes *right_code = codeGen_Exp(root->children[2], right);
            struct InterCodes *assign_code = gen_assign(left, right);
            right_code = concat_codes(right_code, assign_code);
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
        left_codes = concat_codes(left_codes, right_codes);
        left_codes = concat_codes(left_codes, merge_codes);
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
        struct InterCodes *merge_codes = gen_arith(place, zero, right, CG_SUB);

        right_codes = concat_codes(right_codes, merge_codes);
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
        int label2 = new_label();
        struct InterCodes *zero_codes = gen_assign(place, zero);
        struct InterCodes *cond_codes = gen_cond(root, label1, label2);
        struct InterCodes *label1_codes = gen_label(label1);
        struct InterCodes *one_codes = gen_assign(place, one);
        struct InterCodes *label2_codes = gen_label(label2);
        zero_codes = concat_codes(zero_codes, cond_codes);
        zero_codes = concat_codes(zero_codes, label1_codes);
        zero_codes = concat_codes(zero_codes, one_codes);
        zero_codes = concat_codes(zero_codes, label2_codes);
        return zero_codes;
    }
    break;

    case SM_Exp_LPERP:
    {
        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *right_codes = codeGen_Exp(root->children[1], right);
        struct InterCodes *assign_codes = gen_assign(place, right);
        right_codes = concat_codes(right_codes, assign_codes);
        return right_codes;
    }
    break;

    case SM_Exp_ILPRP:
    {
        char *func_name = root->children[0]->str_val;
        if (strcmp(func_name, "read") == 0)
        {
            return gen_read(place);
        }
        return gen_call(place, func_name);
    }
    break;

    case SM_Exp_ILPARP:
    {
        char *func_name = root->children[0]->str_val;
        Sym func_sym=symTable_find(globaltype,func_name,RD_FUNC);
        
        struct InterCodes* args_codes=codeGen_Args(root->children[2],arg_list);
    }
    break;
    //to do
    default:
        assert(0);
        break;
    }
}