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
    case CG_ADDRESS:
    case CG_PARAM:
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
        fprintf(file, "LABEL label%d :", code.u.label_no);
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

    case CG_WRITE:
        fprintf(file, "WRITE ");
        print_operand(code.u.func_call.place, file);
        break;

    case CG_ARG:
        fprintf(file, "ARG ");
        print_operand(code.u.arg, file);
        break;

    case CG_CALL:
        print_operand(code.u.func_call.place, file);
        fprintf(file, " := CALL %s", code.u.func_call.name);
        break;

    case CG_ADDRASSIGN:
        print_operand(code.u.assign.left, file);
        fprintf(file, " := &");
        print_operand(code.u.assign.right, file);
        break;
    case CG_ASSIGN_DEREFER:
        print_operand(code.u.assign.left, file);
        fprintf(file, " := *");
        print_operand(code.u.assign.right, file);
        break;
    case CG_ASSIGN_TOADDR:
        fprintf(file, "*");
        print_operand(code.u.assign.left, file);
        fprintf(file, " := ");
        print_operand(code.u.assign.right, file);
        break;
    case CG_MALLOC:
        fprintf(file, "DEC v%d %d", abs(code.u.malloc.var_no), code.u.malloc.size);
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
        if (func_param->param->kind == RD_VARIABLE || func_param->param->kind == RD_ARRAY_VARIABLE)
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
    return ret;
}

struct InterCodes *gen_write(Operand place)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_WRITE;
    ret->code.u.func_call.place = place;
    return ret;
}

struct InterCodes *gen_arg(Operand arg)
{
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_ARG;
    ret->code.u.arg = arg;
    return ret;
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
    return ret;
}

struct InterCodes *gen_addrAssign(Operand left, Operand right)
{
    assert(left->kind !=CG_VARIABLE && right->kind == CG_VARIABLE);
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_ADDRASSIGN;
    ret->code.u.assign.left = left;
    ret->code.u.assign.right = right;
    return ret;
}

struct InterCodes *gen_assignToAddr(Operand left, Operand right)
{
    assert(left->kind == CG_ADDRESS);
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_ASSIGN_TOADDR;
    ret->code.u.assign.left = left;
    ret->code.u.assign.right = right;
    return ret;
}

struct InterCodes *gen_assignDereference(Operand left, Operand right)
{
    //assert(left->kind == CG_VARIABLE && right->kind != CG_VARIABLE);
    struct InterCodes *ret = new_codes();
    ret->code.kind = CG_ASSIGN_DEREFER;
    ret->code.u.assign.left = left;
    ret->code.u.assign.right = right;
    return ret;
}

struct InterCodes *gen_malloc(int var_no, int size)
{
    struct InterCodes *codes = new_codes();
    codes->code.kind = CG_MALLOC;
    codes->code.u.malloc.size = size;
    codes->code.u.malloc.var_no = var_no;
    return codes;
}

struct InterCodes *codeGen(struct ASTNode *root)
{
    /*
    printf("############################\n");
    symTable_print(globaltype);
    printf("############################\n");
    symTable_print(variables);
    printf("############################\n");
    */
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
        return NULL;
        break;
    }
}

struct InterCodes *codeGen_CompSt(struct ASTNode *root)
{
    struct InterCodes *compst = codeGen_DefList(root->children[1]);
    compst = concat_codes(compst, codeGen_StmtList(root->children[2]));
    return compst;
}

struct InterCodes *codeGen_DefList(struct ASTNode *root)
{
    if (!root)
        return NULL;
    struct InterCodes *deflist = codeGen_DecList(root->children[0]->children[1]);
    deflist = concat_codes(deflist, codeGen_DefList(root->children[1]));
    return deflist;
};

struct InterCodes *codeGen_DecList(struct ASTNode *root)
{
    switch (root->type)
    {
    case SM_DecList_D:
    {
        return codeGen_Dec(root->children[0]);
    }
    break;
    case SM_DecList_DCD:
    {
        struct InterCodes *declist = codeGen_Dec(root->children[0]);
        declist = concat_codes(declist, codeGen_DecList(root->children[2]));
    }
    break;
    default:
        assert(0);
        break;
    }
}

struct InterCodes *codeGen_Dec(struct ASTNode *root)
{
    if (root->type == SM_Dec_V)
    {
        return codeGen_VarDec(root->children[0]);
    }
    else
    {
        assert(root->children[0]->type == SM_VarDec_I);
        Sym v_sym = symTable_find(variables, root->children[0]->children[0]->str_val, RD_VARIABLE);
        v_sym->var_no = new_tmp();
        Operand left = new_operand(CG_VARIABLE);
        left->u.var_no = v_sym->var_no;

        Operand place = new_operand(CG_VARIABLE);
        place->u.var_no = new_tmp();
        struct InterCodes *exp_codes = codeGen_Exp(root->children[2], place);

        struct InterCodes *assign_codes = gen_assign(left, place);
        exp_codes = concat_codes(exp_codes, assign_codes);
    }
}

struct InterCodes *codeGen_VarDec(struct ASTNode *root)
{
    if (root->type == SM_VarDec_I)
    {
        Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
        if (v->u.type_sym->u.type->kind == RD_STRUCTURE)
        {
            v->var_no = new_param();
            return gen_malloc(v->var_no, v->u.type_sym->u.type->size);
        }
        return NULL;
    }
    else
    {
        while (root->type == SM_VarDec_VLIR)
            root = root->children[0];
        Sym v = symTable_find(variables, root->children[0]->str_val, RD_VARIABLE);
        v->var_no = new_param();
        return gen_malloc(v->var_no, v->u.type_sym->u.type->size);
    }
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
            place->u.addr.type_sym = v->u.type_sym;
            if (v->isparam || v->u.type_sym->u.type->kind == RD_BASIC)
            {
                return gen_assign(place, right);
            }
            else
            {
                return gen_addrAssign(place, right);
            }
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
        if (root->children[0]->type == SM_Exp_ID)
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

            if (place)
            {
                right_code = concat_codes(right_code, gen_assign(place, left));
            }
            return right_code;
        }
        else
        {
            Operand left = new_operand(CG_ADDRESS);
            struct InterCodes *left_codes = codeGen_Exp(root->children[0], left);

            switch (left->u.addr.type_sym->u.type->kind)
            {
            case RD_ARRAY:
                //to do
                assert(0);
                break;
            case RD_BASIC:
            {
                Operand right = new_operand(CG_VARIABLE);
                right->u.var_no = new_tmp();
                struct InterCodes *right_codes = codeGen_Exp(root->children[2], right);
                struct InterCodes *assign_codes = gen_assignToAddr(left, right);
                left_codes = concat_codes(left_codes, right_codes);
                left_codes = concat_codes(left_codes, assign_codes);
                return left_codes;
            }
            break;
            default:
                assert(0);
                break;
            }
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
        Sym func_sym = symTable_find(globaltype, func_name, RD_FUNC);
        Operand *arg_list = (Operand *)malloc(sizeof(struct Operand_) * func_sym->u.func_type->n_param);
        struct InterCodes *genargs_codes = codeGen_Args(root->children[2], arg_list, 0);
        if (strcmp(func_name, "write") == 0)
        {
            struct InterCodes *write_codes = gen_write(arg_list[0]);
            genargs_codes = concat_codes(genargs_codes, write_codes);
            if (place)
                genargs_codes = concat_codes(genargs_codes, gen_assign(place, zero));
            return genargs_codes;
        }
        else
        {
            for (int i = func_sym->u.func_type->n_param - 1; i >= 0; i--)
            {
                struct InterCodes *arg_codes = gen_arg(arg_list[i]);
                genargs_codes = concat_codes(genargs_codes, arg_codes);
            }
            genargs_codes = concat_codes(genargs_codes, gen_call(place, func_name));
            return genargs_codes;
        }
    }
    break;

    case SM_Exp_ELBERB:
    {
        Operand left = new_operand(CG_ADDRESS);
        left->u.addr.var_no = new_tmp();
        struct InterCodes *left_codes = codeGen_Exp(root->children[0], left);

        Operand right = new_operand(CG_VARIABLE);
        right->u.var_no = new_tmp();
        struct InterCodes *right_codes = codeGen_Exp(root->children[2], right);
        left_codes = concat_codes(left_codes, right_codes);

        Operand offset = new_operand(CG_VARIABLE);
        offset->u.var_no = new_tmp();
        Operand size = new_operand(CG_CONSTANT);
        Sym next_sym = left->u.addr.type_sym->u.type->u.array_ty.next;
        if (next_sym)
        {
            size->u.value = next_sym->u.type->size;
        }
        else
        {
            size->u.value = left->u.addr.type_sym->u.type->u.array_ty.elem->u.type->size;
        }
        left_codes = concat_codes(left_codes, gen_arith(offset, right, size, CG_MUL));

        struct InterCodes *addr_codes = gen_arith(left, left, offset, CG_ADD);
        left_codes = concat_codes(left_codes, addr_codes);

        struct InterCodes *derefernce_codes = NULL;
        if (place->kind == CG_VARIABLE || (place->kind == CG_PARAM && left->u.addr.type_sym->u.type->u.array_ty.next == NULL && left->u.addr.type_sym->u.type->u.array_ty.elem->u.type->kind == RD_BASIC))
        {
            //right value
            derefernce_codes = gen_assignDereference(place, left);
        }
        else
        {
            destroy_tmp(place->u.addr.var_no);
            place->u.addr.var_no = left->u.addr.var_no;
            place->u.addr.type_sym = left->u.addr.type_sym->u.type->u.array_ty.next;
            if (place->u.addr.type_sym == NULL)
            {
                place->u.addr.type_sym = left->u.addr.type_sym->u.type->u.array_ty.elem;
            }
        }
        left_codes = concat_codes(left_codes, derefernce_codes);
    }
    break;

    case SM_Exp_EDI:
    {
        Operand left = new_operand(CG_ADDRESS);
        left->u.addr.var_no = new_tmp();
        struct InterCodes *left_codes = codeGen_Exp(root->children[0], left);
        FieldList field = structType_findFeild_retfield(left->u.addr.type_sym, root->children[2]->str_val);

        struct InterCodes *addr_codes = NULL;
        if (field->offset > 0)
        {
            Operand offset = new_operand(CG_CONSTANT);
            offset->u.value = field->offset;
            addr_codes = gen_arith(left, left, offset, CG_ADD);
        }
        struct InterCodes *derefernce_codes = NULL;
        if (place->kind == CG_VARIABLE || (place->kind == CG_PARAM && field->sym->u.type_sym->u.type->kind == RD_BASIC))
        {
            //right value
            derefernce_codes = gen_assignDereference(place, left);
        }
        else
        {
            destroy_tmp(place->u.addr.var_no);
            place->u.addr.var_no = left->u.addr.var_no;
            place->u.addr.type_sym = field->sym->u.type_sym;
        }
        left_codes = concat_codes(left_codes, addr_codes);
        left_codes = concat_codes(left_codes, derefernce_codes);
        return left_codes;
    }
    break;
    //to do
    default:
        assert(0);
        break;
    }
}

struct InterCodes *codeGen_Args(struct ASTNode *root, Operand *arg_list, int i)
{
    switch (root->type)
    {
    case SM_Args_ECA:
    {
        Operand place = new_operand(CG_PARAM);
        place->u.var_no = new_tmp();
        struct InterCodes *exp_codes = codeGen_Exp(root->children[0], place);
        arg_list[i] = place;

        struct InterCodes *args_codes = codeGen_Args(root->children[2], arg_list, i + 1);
        exp_codes = concat_codes(exp_codes, args_codes);
        return exp_codes;
    }
    break;

    case SM_Args_E:
    {
        Operand place = new_operand(CG_PARAM);
        place->u.var_no = new_tmp();
        struct InterCodes *exp_codes = codeGen_Exp(root->children[0], place);
        arg_list[i] = place;
        return exp_codes;
    }
    break;

    default:
        assert(0);
        break;
    }
}