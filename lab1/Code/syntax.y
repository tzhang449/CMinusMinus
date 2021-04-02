%{
    #include "ast.h"
    #include "lex.yy.c"

    extern struct ASTNode *root;
    extern int hasError;
    int previousErrLine=0;

    int yyerror(const char*);
%}
%error-verbose
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
        root=newNode("Program", ISOTHER, @$.first_line);
        insert(root,$1);
    };

ExtDefList : ExtDef ExtDefList {
        $$=newNode("ExtDefList",ISOTHER,@$.first_line);
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
        $$=newNode("ExtDef",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Specifier SEMI {
        $$=newNode("ExtDef",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | Specifier FunDec CompSt {
        $$=newNode("ExtDef",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Specifier error{
        yyerrok;
    }
    | error SEMI {
        yyerrok;
    };

ExtDecList : VarDec {
        $$=newNode("ExtDecList",ISOTHER,@$.first_line);
        insert($$,$1);
    }
    | VarDec COMMA ExtDecList {
        $$=newNode("ExtDecList",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    };

/* Specifiers */
Specifier : TYPE {
        $$=newNode("Specifier",ISOTHER,@$.first_line);
        insert($$,$1);
    }
    | StructSpecifier {
        $$=newNode("Specifier",ISOTHER,@$.first_line);
        insert($$,$1);
    };

StructSpecifier : STRUCT OptTag LC DefList RC {
        $$=newNode("StructSpecifier",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
        insert($$,$5);
    }
    | STRUCT Tag {
        $$=newNode("StructSpecifier",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    };

OptTag : ID {
        $$=newNode("OptTag",ISOTHER,@$.first_line);
        insert($$,$1);  
    }
    | /*empty*/ {
        $$=NULL;
    };

Tag : ID {
        $$=newNode("Tag",ISOTHER,@$.first_line);
        insert($$,$1);  
    };

/* Declarators */
VarDec : ID {
        $$=newNode("VarDec",ISOTHER,@$.first_line);
        insert($$,$1);  
    }
    | VarDec LB INT RB {
        $$=newNode("VarDec",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    };

FunDec : ID LP VarList RP {
        $$=newNode("FunDec",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    }
    | ID LP RP {
        $$=newNode("FunDec",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
    }
    | error RP {
        yyerrok;
    };

VarList : ParamDec COMMA VarList {
        $$=newNode("VarList",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
    }
    | ParamDec {
        $$=newNode("VarList",ISOTHER,@$.first_line);
        insert($$,$1); 
    };

ParamDec : Specifier VarDec {
        $$=newNode("ParamDec",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    };

/* Statements */
CompSt : LC DefList StmtList RC {
        $$=newNode("CompSt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
        insert($$,$4);  
    }
    | error RC {
        yyerrok;
    };

StmtList : Stmt StmtList {
        $$=newNode("StmtList",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }
    | /*empty*/ {
        $$=NULL;
    };

Stmt : Exp SEMI {
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }
    | CompSt {
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
    }
    | RETURN Exp SEMI {
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
        insert($$,$6);
        insert($$,$7); 
    }
    | WHILE LP Exp RP Stmt {
        $$=newNode("Stmt",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3); 
        insert($$,$4);
        insert($$,$5); 
    }
    | error SEMI {
        yyerrok;
    };

/* Local Definitions */
DefList : Def DefList {
        $$=newNode("DefList",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
    }    
    | /*empty*/ {
        $$=NULL;
    };

Def : Specifier DecList SEMI {
        $$=newNode("Def",ISOTHER,@$.first_line);
        insert($$,$1); 
        insert($$,$2); 
        insert($$,$3);
    }
    | error SEMI {
        yyerrok;
    };

DecList : Dec {
        $$=newNode("DecList",ISOTHER,@$.first_line);
        insert($$,$1); 
    }
    | Dec COMMA DecList {
        $$=newNode("DecList",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    };

Dec : VarDec {
        $$=newNode("Dec",ISOTHER,@$.first_line);
        insert($$,$1);
    }
    | VarDec ASSIGNOP Exp {
        $$=newNode("Dec",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    };

/* Expressions */
Exp : Exp ASSIGNOP Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp AND Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp OR Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp RELOP Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp PLUS Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp MINUS Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp STAR Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp DIV Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | LP Exp RP {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | MINUS Exp %prec NEG{
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | NOT Exp {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
    }
    | ID LP Args RP {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
    }
    | ID LP RP {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp LB Exp RB {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
        insert($$,$4);
    }
    | Exp DOT ID {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | ID {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
    }
    | INT {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
    }
    | FLOAT {
        $$=newNode("Exp",ISOTHER,@$.first_line);
        insert($$,$1);
    };

Args : Exp COMMA Args {
        $$=newNode("Args",ISOTHER,@$.first_line);
        insert($$,$1);
        insert($$,$2);
        insert($$,$3);
    }
    | Exp {$$=newNode("Args",ISOTHER,@$.first_line);
        insert($$,$1);};
%%
int yyerror(const char *msg){
    hasError++;
    if(previousErrLine !=yylloc.first_line){
        fprintf(stderr, "Error type B at Line %d: %s.\n",yylloc.first_line,msg);
        previousErrLine=yylloc.first_line;
    }
}