%{
    #include <stdio.h>
    #include "syntax.tab.h"

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

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
    sscanf(yytext,"%o",&yylval.int_val);
    return INT;
}

{DECINT} {
    sscanf(yytext,"%d",&yylval.int_val);
    return INT;
}

{HEXINT} {
    sscanf(yytext,"%x",&yylval.int_val);
    return INT;
}

{FLOAT} {
    sscanf(yytext,"%f",&yylval.float_val);
    return FLOAT;
}

{SEMI} {
    return SEMI;
}

{COMMA} {
    return COMMA;
}

{ASSIGNOP} {
    return ASSIGNOP;
}

{RELOP} {
    return RELOP;
}

{PLUS} {
    return PLUS;
}

{MINUS} {
    return MINUS;
}

{STAR} {
    return STAR;
}

{DIV} {
    return DIV;
}

{AND} {
    return AND;
}

{OR} {
    return OR;
}

{DOT} {
    return DOT;
}

{NOT} {
    return NOT;
}

{TYPE} {
    return TYPE;
}

{LP} {
    return LP;
}

{RP} {
    return RP;
}

{LB} {
    return LB;
}

{RB} {
    return RB;
}

{LC} {
    return LC;
}

{RC} {
    return RC;
}

{STRUCT} {
    return STRUCT;
}

{RETURN} {
    return RETURN;
}

{IF} {
    return IF;
}

{ELSE} {
    return ELSE;
}


{WHILE} {
    return WHILE;
}

{ID} {
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
    printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext);
}

%%