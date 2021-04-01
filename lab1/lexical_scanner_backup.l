%{
    #include <stdio.h>
    int n=0;
    float f=0;
%}

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
    sscanf(yytext,"%o",&n);
    printf(" OCTINT:%d ",n);
}

{DECINT} {
    sscanf(yytext,"%d",&n);
    printf(" DECINT:%d ",n);
}

{HEXINT} {
    sscanf(yytext,"%x",&n);
    printf(" HEXINT:%d ",n);
}

{FLOAT} {
    sscanf(yytext,"%f",&f);
    printf(" FLOAT:%f ",f);
}

{SEMI} {
    printf(" SEMI ");
}

{COMMA} {
    printf(" COMMA ");
}

{ASSIGNOP} {
    printf(" ASSIGNOP ");
}

{RELOP} {
    printf(" RELOP ");
}

{PLUS} {
    printf(" PLUS ");
}

{MINUS} {
    printf(" MINUS ");
}

{STAR} {
    printf(" STAR ");
}

{DIV} {
    printf(" DIV ");
}

{AND} {
    printf(" AND ");
}

{OR} {
    printf(" OR ");
}

{DOT} {
    printf(" DOT ");
}

{NOT} {
    printf(" NOT ");
}

{TYPE} {
    printf(" TYPE ");
}

{LP} {
    printf(" LP ");
}

{RP} {
    printf(" RP ");
}

{LB} {
    printf(" LB ");
}

{RB} {
    printf(" RB ");
}

{LC} {
    printf(" LC ");
}

{RC} {
    printf(" RC ");
}

{STRUCT} {
    printf(" STRUCt ");
}

{RETURN} {
    printf(" RETURN ");
}

{IF} {
    printf(" IF ");
}

{ELSE} {
    printf(" ELSE ");
}


{WHILE} {
    printf(" WHILE ");
}

{ID} {
    printf(" ID ");
}

"//"[^\n]*\n {
    printf(" COMMENT\n");
}

"/*"([^*]|(\*+[^*/]))*\*+\/ {
    printf(" COMMENT ");
}

\n {
    printf("\n");
}

[ \r\t] {
}

. {  
    printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext);
}

%%