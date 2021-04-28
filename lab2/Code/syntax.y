%{
    #include "ast.h"
    #include "lex.yy.c"

    extern struct ASTNode *root;
    extern int hasError;
    int previousErrLine=0;

    int yyerror(const char*);
%}
%define parse.error verbose
/* declared types */
%union{
    struct ASTNode *Node;
}

/* declared tokens */
%token <Node> INT FLOAT ID TYPE
%token <Node> SEMI COMMA
%token <Node> ASSIGNOP RELOP PLUS MINUS STAR DIV 
%token <Node> AND OR DOT NOT
%token <Node> LP RP LB RB LC RC
%token <Node> STRUCT RETURN IF ELSE WHILE

%type <Node> Program ExtDefList ExtDef ExtDecList
%type <Node> Specifier StructSpecifier OptTag Tag
%type <Node> VarDec FunDec VarList ParamDec
%type <Node> CompSt StmtList Stmt 
%type <Node> DefList Def DecList Dec
%type <Node> Exp Args

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%precedence NEG 
%left LP RP LB RB DOT

%%
/* High-level Definitions */
Program : ExtDefList {
        root=newNode("Program", SM_Program, @$.first_line);
        insert(root,$1);
    };

ExtDefList : ExtDef ExtDefList {
        $$=newNode("ExtDefList",SM_ExtDefList,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | /*empty*/ {
        $$=NULL;
    }
    | error ExtDefList {
        yyerrok;
    };

ExtDef : Specifier ExtDecList SEMI {
        $$=newNode("ExtDef",SM_ExtDef_SES,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Specifier SEMI {
        $$=newNode("ExtDef",SM_ExtDef_SS,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | Specifier FunDec CompSt {
        $$=newNode("ExtDef",SM_ExtDef_SFC,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Specifier error SEMI{
        yyerrok;
    }
    | error SEMI {
        yyerrok;
    };

ExtDecList : VarDec {
        $$=newNode("ExtDecList",SM_ExtDecList_V,@$.first_line);
        insert($$,$1);
    }
    | VarDec COMMA ExtDecList {
        $$=newNode("ExtDecList",SM_ExtDecList_VCE,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | error ExtDecList{
        yyerrok;
    };

/* Specifiers */
Specifier : TYPE {
        $$=newNode("Specifier",SM_Specifiers_T,@$.first_line);
        insert($$,$1);
    }
    | StructSpecifier {
        $$=newNode("Specifier",SM_Specifiers_S,@$.first_line);
        insert($$,$1);
    };

StructSpecifier : STRUCT OptTag LC DefList RC {
        $$=newNode("StructSpecifier",SM_StructSpecifier_SOLDR,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
        insert($$,$5);
    }
    | STRUCT Tag {
        $$=newNode("StructSpecifier",SM_StructSpecifier_ST,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | error RC{
        yyerrok;
    };

OptTag : ID {
        $$=newNode("OptTag",SM_OptTag,@$.first_line);
        insert($$,$1);  
    }
    | /*empty*/ {
        $$=NULL;
    };

Tag : ID {
        $$=newNode("Tag",SM_Tag,@$.first_line);
        insert($$,$1);  
    };

/* Declarators */
VarDec : ID {
        $$=newNode("VarDec",SM_VarDec_I,@$.first_line);
        insert($$,$1);  
    }
    | VarDec LB INT RB {
        $$=newNode("VarDec",SM_VarDec_VLIR,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    }
    | error RB{
        yyerrok;
    };

FunDec : ID LP VarList RP {
        $$=newNode("FunDec",SM_FunDec_ILVR,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    }
    | ID LP RP {
        $$=newNode("FunDec",SM_FunDec_ILR,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
    }
    | error RP {
        yyerrok;
    };

VarList : ParamDec COMMA VarList {
        $$=newNode("VarList",SM_VarList_PCV,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
    }
    | ParamDec {
        $$=newNode("VarList",SM_VarList_P,@$.first_line);
        insert($$,$1); 
    }
    | error VarList{
        yyerrok;
    };

ParamDec : Specifier VarDec {
        $$=newNode("ParamDec",SM_ParamDec,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }
    | error VarDec{
        yyerrok;
    };

/* Statements */
CompSt : LC DefList StmtList RC {
        $$=newNode("CompSt",SM_CompSt,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    }
    | error RC {
        yyerrok;
    };

StmtList : Stmt StmtList {
        $$=newNode("StmtList",SM_StmtList,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }
    | /*empty*/ {
        $$=NULL;
    };

Stmt : Exp SEMI {
        $$=newNode("Stmt",SM_Stmt_ES,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }
    | CompSt {
        $$=newNode("Stmt",SM_Stmt_C,@$.first_line);
        insert($$,$1); 
    }
    | RETURN Exp SEMI {
        $$=newNode("Stmt",SM_Stmt_RES,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{
        $$=newNode("Stmt",SM_Stmt_ILERS,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$=newNode("Stmt",SM_Stmt_ILERSES,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
        insert($$,$6);
        insert($$,$7); 
    }
    | WHILE LP Exp RP Stmt {
        $$=newNode("Stmt",SM_Stmt_WLERS,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
    }
    | IF LP error RP Stmt {
        yyerrok;
    }
    | error SEMI {
        yyerrok;
    };

/* Local Definitions */
DefList : Def DefList {
        $$=newNode("DefList",SM_DefList,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }    
    | /*empty*/ {
        $$=NULL;
    };

Def : Specifier DecList SEMI {
        $$=newNode("Def",SM_Def,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
    }
    | error SEMI {
        yyerrok;
    };

DecList : Dec {
        $$=newNode("DecList",SM_DecList_D,@$.first_line);
        insert($$,$1); 
    }
    | Dec COMMA DecList {
        $$=newNode("DecList",SM_DecList_DCD,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | error DecList{
        yyerrok;
    };

Dec : VarDec {
        $$=newNode("Dec",SM_Dec_V,@$.first_line);
        insert($$,$1);
    }
    | VarDec ASSIGNOP Exp {
        $$=newNode("Dec",SM_Dec_VAE,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    };

/* Expressions */
Exp : Exp ASSIGNOP Exp {
        $$=newNode("Exp",SM_Exp_ASSIGN,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp AND Exp {
        $$=newNode("Exp",SM_Exp_AND,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp OR Exp {
        $$=newNode("Exp",SM_Exp_OR,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp RELOP Exp {
        $$=newNode("Exp",SM_Exp_RELOP,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp PLUS Exp {
        $$=newNode("Exp",SM_Exp_PLUS,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp MINUS Exp {
        $$=newNode("Exp",SM_Exp_MINUS,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp STAR Exp {
        $$=newNode("Exp",SM_Exp_STAR,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp DIV Exp {
        $$=newNode("Exp",SM_Exp_DIV,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | LP Exp RP {
        $$=newNode("Exp",SM_Exp_LPERP,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | MINUS Exp %prec NEG{
        $$=newNode("Exp",SM_Exp_MINUSE,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | NOT Exp {
        $$=newNode("Exp",SM_Exp_NOTE,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | ID LP Args RP {
        $$=newNode("Exp",SM_Exp_ILPARP,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
    }
    | ID LP RP {
        $$=newNode("Exp",SM_Exp_ILPRP,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp LB Exp RB {
        $$=newNode("Exp",SM_Exp_ELBERB,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
    }
    | Exp DOT ID {
        $$=newNode("Exp",SM_Exp_EDI,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | ID {
        $$=newNode("Exp",SM_Exp_ID,@$.first_line);
        insert($$,$1);
    }
    | INT {
        $$=newNode("Exp",SM_Exp_INT,@$.first_line);
        insert($$,$1);
    }
    | FLOAT {
        $$=newNode("Exp",SM_Exp_FLOAT,@$.first_line);
        insert($$,$1);
    };

Args : Exp COMMA Args {
        $$=newNode("Args",SM_Args_ECA,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp {$$=newNode("Args",SM_Args_E,@$.first_line);
        insert($$,$1);};
%%
int yyerror(const char *msg){
    hasError++;
    if(previousErrLine!=yylloc.first_line){
        fprintf(stderr, "Error type B at Line %d: %s.\n",yylloc.first_line,msg);
        previousErrLine=yylloc.first_line;
    }
}