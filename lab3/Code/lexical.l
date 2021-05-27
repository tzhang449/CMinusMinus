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
    yylval.Node=newNode("INT",SM_INT,yylineno);
    sscanf(yytext,"%o",&yylval.Node->int_val);
    return INT;
}

{DECINT} {
    yylval.Node=newNode("INT",SM_INT,yylineno);
    sscanf(yytext,"%d",&yylval.Node->int_val);
    return INT;
}

{HEXINT} {
    yylval.Node=newNode("INT",SM_INT,yylineno);
    sscanf(yytext,"%x",&yylval.Node->int_val);
    return INT;
}

{FLOAT} {
    yylval.Node=newNode("FLOAT",SM_FLOAT,yylineno);
    sscanf(yytext,"%f",&yylval.Node->float_val);
    return FLOAT;
}

{SEMI} {
    yylval.Node=newNode("SEMI",SM_OTHERTERM,yylineno);
    return SEMI;
}

{COMMA} {
    yylval.Node=newNode("COMMA",SM_OTHERTERM,yylineno);
    return COMMA;
}

{ASSIGNOP} {
    yylval.Node=newNode("ASSIGNOP",SM_OTHERTERM,yylineno);
    return ASSIGNOP;
}

{RELOP} {
    yylval.Node=newNode("RELOP",SM_OTHERTERM,yylineno);
    strcpy(yylval.Node->str_val,yytext);
    return RELOP;
}

{PLUS} {
    yylval.Node=newNode("PLUS",SM_OTHERTERM,yylineno);
    return PLUS;
}

{MINUS} {
    yylval.Node=newNode("MINUS",SM_OTHERTERM,yylineno);
    return MINUS;
}

{STAR} {
    yylval.Node=newNode("STAR",SM_OTHERTERM,yylineno);
    return STAR;
}

{DIV} {
    yylval.Node=newNode("DIV",SM_OTHERTERM,yylineno);
    return DIV;
}

{AND} {
    yylval.Node=newNode("AND",SM_OTHERTERM,yylineno);
    return AND;
}

{OR} {
    yylval.Node=newNode("OR",SM_OTHERTERM,yylineno);
    return OR;
}

{DOT} {
    yylval.Node=newNode("DOT",SM_OTHERTERM,yylineno);
    return DOT;
}

{NOT} {
    yylval.Node=newNode("NOT",SM_OTHERTERM,yylineno);
    return NOT;
}

{TYPE} {
    yylval.Node=newNode("TYPE",SM_TYPE,yylineno);
    if (strcmp(yytext,"int")==0){
        yylval.Node->type_val=RD_INT;
    }else{
        yylval.Node->type_val=RD_FLOAT;
    }
    return TYPE;
}

{LP} {
    yylval.Node=newNode("LP",SM_OTHERTERM,yylineno);
    return LP;
}

{RP} {
    yylval.Node=newNode("RP",SM_OTHERTERM,yylineno);
    return RP;
}

{LB} {
    yylval.Node=newNode("LB",SM_OTHERTERM,yylineno);
    return LB;
}

{RB} {
    yylval.Node=newNode("RB",SM_OTHERTERM,yylineno);
    return RB;
}

{LC} {
    yylval.Node=newNode("LC",SM_OTHERTERM,yylineno);
    return LC;
}

{RC} {
    yylval.Node=newNode("RC",SM_OTHERTERM,yylineno);
    return RC;
}

{STRUCT} {
    yylval.Node=newNode("STRUCT",SM_OTHERTERM,yylineno);
    return STRUCT;
}

{RETURN} {
    yylval.Node=newNode("RETURN",SM_OTHERTERM,yylineno);
    return RETURN;
}

{IF} {
    yylval.Node=newNode("IF",SM_OTHERTERM,yylineno);
    return IF;
}

{ELSE} {
    yylval.Node=newNode("ELSE",SM_OTHERTERM,yylineno);
    return ELSE;
}


{WHILE} {
    yylval.Node=newNode("WHILE",SM_OTHERTERM,yylineno);
    return WHILE;
}

{ID} {
    yylval.Node=newNode("ID",SM_ID,yylineno);
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