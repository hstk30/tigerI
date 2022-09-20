%{
#include <stdio.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char const *s) {
    EM_error(EM_tokPos, "%s", s);
}
%}

/* %define parse.error detailed */

%union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
    A_dec dec;
    A_ty ty;
    A_field field;
    A_fieldList field_list;
    A_expList exp_list;
    A_fundec fundec;
    A_fundecList fundec_list;
    A_decList dec_list;
    A_namety namety;
    A_nametyList namety_list;
    A_efield efield;
    A_efieldList efield_list;
	}


%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%token <sval> ID STRING
%token <ival> INT

%type <var> lvalue
%type <exp> program exp 
%type <exp_list> args args_ expseq expseq_
%type <efield_list> efield_list efield_list_ 
%type <efield> efield
%type <dec_list> decs
%type <dec> dec vardec 
%type <namety_list> tydec_list 
%type <namety> tydec
%type <ty> ty
%type <field_list> tyfields tyfields_ 
%type <field> tyfield
%type <fundec_list> fundec_list
%type <fundec> fundec

/* fix tydec_list fundec_list shift/reduce conflict  */
%nonassoc LOWEST
%nonassoc TYPE FUNCTION

%nonassoc ID
%nonassoc LBRACK

%nonassoc THEN
%nonassoc ELSE

%nonassoc DO OF

%nonassoc ASSIGN
%left AND OR
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS 
%left TIMES DIVIDE 
%left UMINUS

%start program

%%

program:   exp    {absyn_root=$1;}

exp:   
        lvalue  {$$ = A_VarExp(EM_tokPos, $1);}
    |   NIL     {$$ = A_NilExp(EM_tokPos);}
    |   INT     {$$ = A_IntExp(EM_tokPos, $1);}
    |   STRING  {$$ = A_StringExp(EM_tokPos, $1);}
    |   ID LPAREN args RPAREN   {$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}

    |   exp AND exp 	{$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}
    |   exp OR exp 	{$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos,1), $3);}

    |   MINUS exp %prec UMINUS  {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
    |   exp EQ exp 	{$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
    |   exp NEQ exp 	{$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
    |   exp PLUS exp	{$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
    |   exp MINUS exp	{$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
    |   exp TIMES exp	{$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
    |   exp DIVIDE exp {$$ = A_OpExp(EM_tokPos, A_divideOp, $1,$3);}
    |   exp GE exp	{$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}
    |   exp LE exp	{$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
    |   exp GT exp	{$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
    |   exp LT exp	{$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}

    |   ID LBRACE efield_list RBRACE    {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}
    |   LPAREN expseq RPAREN {$$ = A_SeqExp(EM_tokPos, $2);}
    |   lvalue ASSIGN exp   {$$ = A_AssignExp(EM_tokPos, $1, $3);}

    |   IF exp THEN exp     {$$ = A_IfExp(EM_tokPos, $2, $4, NULL);}
    |   IF exp THEN exp ELSE exp    {$$ = A_IfExp(EM_tokPos, $2, $4, $6);}

    |   WHILE exp DO exp    {$$ = A_WhileExp(EM_tokPos, $2, $4);}
    |   FOR ID ASSIGN exp TO exp DO exp {$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}
    |   BREAK   {$$ = A_BreakExp(EM_tokPos);}

    |   LET decs IN expseq END  {$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));}
    |   ID LBRACK exp RBRACK OF exp {$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);}

lvalue: 
        ID                          {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
    |   lvalue DOT ID               {$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
    |   lvalue LBRACK exp RBRACK    {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}
    |   ID LBRACK exp RBRACK        {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}

args: 
        /* empty */     {$$ = NULL;}
    |   args_           {$$ = $1;}

args_:
        exp             {$$ = A_ExpList($1, NULL);} 
    |   exp COMMA args  {$$ = A_ExpList($1, $3);}

efield_list:
        /* empty */     {$$ = NULL;}
    |   efield_list_     {$$ = $1;}

efield_list_:
        efield                      {$$ = A_EfieldList($1, NULL);}
    |   efield COMMA efield_list_   {$$ = A_EfieldList($1, $3);}

efield:
        ID EQ exp   {$$ = A_Efield(S_Symbol($1), $3);}

expseq:
        /* empty */             {$$ = NULL;}
    |   expseq_                 {$$ = $1;} 

expseq_:
        exp                     {$$ = A_ExpList($1, NULL);}
    |   exp SEMICOLON expseq_    {$$ = A_ExpList($1, $3);}
        
decs:
        /* empty */ {$$ = NULL;}
    |   dec decs    {$$ = A_DecList($1, $2);}

dec:
        vardec           {$$ = $1;}
    |   tydec_list       {$$ = A_TypeDec(EM_tokPos, $1);}
    |   fundec_list      {$$ = A_FunctionDec(EM_tokPos, $1);}

vardec:
        VAR ID ASSIGN exp           {$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}
    |   VAR ID COLON ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}

tydec_list: 
        tydec %prec LOWEST  {$$ = A_NametyList($1, NULL);}
    |   tydec tydec_list    {$$ = A_NametyList($1, $2);}

tydec:
        TYPE ID EQ ty   {$$ = A_Namety(S_Symbol($2), $4);}

ty:
        ID                      {$$ = A_NameTy(EM_tokPos, S_Symbol($1));}
    |   LBRACE tyfields RBRACE  {$$ = A_RecordTy(EM_tokPos, $2);}
    |   ARRAY OF ID             {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}

tyfields:
        /* empty */     {$$ = NULL;}
    |   tyfields_       {$$ = $1;} 

tyfields_:
        tyfield                 {$$ = A_FieldList($1, NULL);}
    |   tyfield COMMA tyfields_ {$$ = A_FieldList($1, $3);}

tyfield:
        ID COLON ID     {$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));}
        
fundec_list:
        fundec %prec LOWEST     {$$ = A_FundecList($1, NULL);}
    |   fundec fundec_list      {$$ = A_FundecList($1, $2);}

fundec:
        FUNCTION ID LPAREN tyfields RPAREN EQ exp           {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}
    |   FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp  {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}

