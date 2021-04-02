%{
    #include "stdio.h"
    #include "string.h"

    #include "ast.h"
    #include "syntax.tab.h"

    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

    extern int hasError;
    int yycolumn = 1;
    int int_val;
    float float_val;
    char str_val[32];
%}

/* this variable stores the line number */
%option yylineno


OCTINT 0[0-7]+
DECINT 0|([1-9][0-9]*)
HEXINT 0[Xx][0-9A-Fa-f]+

FLOAT ([0-9]+"."[0-9]+)|(([0-9]+"."[0-9]*|[0-9]*"."[0-9]+)[Ee][+-]?[0-9]+)

ID [_A-Za-z][_A-Za-z0-9]*

SEMI ;
COMMA ,
ASSIGNOP =
RELOP >|<|>=|<=|==|!=
PLUS "+"
MINUS -
STAR "*"
DIV "/"
AND &&
OR "||"
DOT "."
NOT !
TYPE int|float
LP "("
RP ")"
LB "["
RB "]"
LC "{"
RC "}"
STRUCT struct
RETURN return
IF if
ELSE else
WHILE while

%%
{OCTINT} {
    yylval.Node=newNode("INT",ISINT,yylineno);
    sscanf(yytext,"%o",&yylval.Node->int_val);
    return INT;
}

{DECINT} {
    yylval.Node=newNode("INT",ISINT,yylineno);
    sscanf(yytext,"%d",&yylval.Node->int_val);
    return INT;
}

{HEXINT} {
    yylval.Node=newNode("INT",ISINT,yylineno);
    sscanf(yytext,"%x",&yylval.Node->int_val);
    return INT;
}

{FLOAT} {
    yylval.Node=newNode("FLOAT",ISFLOAT,yylineno);
    sscanf(yytext,"%f",&yylval.Node->float_val);
    return FLOAT;
}

{SEMI} {
    yylval.Node=newNode("SEMI",ISOTHERTERM,yylineno);
    return SEMI;
}

{COMMA} {
    yylval.Node=newNode("COMMA",ISOTHERTERM,yylineno);
    return COMMA;
}

{ASSIGNOP} {
    yylval.Node=newNode("ASSIGNOP",ISOTHERTERM,yylineno);
    return ASSIGNOP;
}

{RELOP} {
    yylval.Node=newNode("RELOP",ISOTHERTERM,yylineno);
    return RELOP;
}

{PLUS} {
    yylval.Node=newNode("PLUS",ISOTHERTERM,yylineno);
    return PLUS;
}

{MINUS} {
    yylval.Node=newNode("MINUS",ISOTHERTERM,yylineno);
    return MINUS;
}

{STAR} {
    yylval.Node=newNode("STAR",ISOTHERTERM,yylineno);
    return STAR;
}

{DIV} {
    yylval.Node=newNode("DIV",ISOTHERTERM,yylineno);
    return DIV;
}

{AND} {
    yylval.Node=newNode("AND",ISOTHERTERM,yylineno);
    return AND;
}

{OR} {
    yylval.Node=newNode("OR",ISOTHERTERM,yylineno);
    return OR;
}

{DOT} {
    yylval.Node=newNode("DOT",ISOTHERTERM,yylineno);
    return DOT;
}

{NOT} {
    yylval.Node=newNode("NOT",ISOTHERTERM,yylineno);
    return NOT;
}

{TYPE} {
    yylval.Node=newNode("TYPE",ISTYPE,yylineno);
    if (strcmp(yytext,"int")==0){
        yylval.Node->type_val=0;
    }else{
        yylval.Node->type_val=1;
    }
    return TYPE;
}

{LP} {
    yylval.Node=newNode("LP",ISOTHERTERM,yylineno);
    return LP;
}

{RP} {
    yylval.Node=newNode("RP",ISOTHERTERM,yylineno);
    return RP;
}

{LB} {
    yylval.Node=newNode("LB",ISOTHERTERM,yylineno);
    return LB;
}

{RB} {
    yylval.Node=newNode("RB",ISOTHERTERM,yylineno);
    return RB;
}

{LC} {
    yylval.Node=newNode("LC",ISOTHERTERM,yylineno);
    return LC;
}

{RC} {
    yylval.Node=newNode("RC",ISOTHERTERM,yylineno);
    return RC;
}

{STRUCT} {
    yylval.Node=newNode("STRUCT",ISOTHERTERM,yylineno);
    return STRUCT;
}

{RETURN} {
    yylval.Node=newNode("RETURN",ISOTHERTERM,yylineno);
    return RETURN;
}

{IF} {
    yylval.Node=newNode("IF",ISOTHERTERM,yylineno);
    return IF;
}

{ELSE} {
    yylval.Node=newNode("ELSE",ISOTHERTERM,yylineno);
    return ELSE;
}


{WHILE} {
    yylval.Node=newNode("WHILE",ISOTHERTERM,yylineno);
    return WHILE;
}

{ID} {
    yylval.Node=newNode("ID",ISID,yylineno);
    strcpy(yylval.Node->str_val,yytext);
    return ID;
}

"//"[^\n]*\n {

}

"/*"([^*]|(\*+[^*/]))*\*+\/ {

}

\n {
    yycolumn = 1;
}

[ \r\t] {
}

. {  
    hasError++;
    printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext);
}

%%