%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

#define INIT_CAPACITY 32
#define EXTRA_BUFF 32

int charPos=1;

struct FBuffer {
    string str_buff;
    size_t len;
    size_t capacity;
} FBuffer;

FBuffer f_buff;

static void
init_str_buffer() {
    string str_buff = checked_malloc(INIT_CAPACITY);

    f_buff.str_buff = str_buff;
    f_buff.len = 0;
    f_buff.capacity = INIT_CAPACITY;
    str_buff[0] = '\0';
}

static void
str_buffer_append (char c) {
    if (f_buff.len + 1 > f_buff.capacity - 1) {
        string new_buff = checked_malloc(f_buff.capacity + EXTRA_BUFF);
        strcpy(new_buff, f_buff.str_buf);
        free(f_buff.str_buff);

        f_buff.str_buff = new_buff;
        f_buff.capacity = f_buff.capacity + EXTRA_BUFF;
    } else {
        f_buff.str_buff[f_buff.len ++] = c;
        f_buf.str_buff[f_buff.len] = '\0';
    }
}

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

%}

%START INITIAL COMMENT STR

%%

/* ignore characters */
<INITIAL>[ \t\r]	{adjust(); continue;}

/* newline */
<INITIAL>\n       {adjust(); EM_newline(); continue;}

/* punctuation symbols */
<INITIAL>","	  {adjust(); return COMMA;}
<INITIAL>":"    {adjust(); return COLON;}
<INITIAL>";"    {adjust(); return SEMICOLON;}
<INITIAL>"("    {adjust(); return LPAREN;}
<INITIAL>")"    {adjust(); return RPAREN;}
<INITIAL>"["    {adjust(); return LBRACK;}
<INITIAL>"]"    {adjust(); return RBRACK;}
<INITIAL>"{"    {adjust(); return LBRACE;}
<INITIAL>"}"    {adjust(); return RBRACE;}
<INITIAL>"."    {adjust(); return DOT;}
<INITIAL>"+"    {adjust(); return PLUS;}
<INITIAL>"-"    {adjust(); return MINUS;}
<INITIAL>"*"    {adjust(); return TIMES;}
<INITIAL>"/"    {adjust(); return DIVIDE;}
<INITIAL>"="    {adjust(); return EQ;}
<INITIAL>"<>"   {adjust(); return NEQ;}
<INITIAL>"<"    {adjust(); return LT;}
<INITIAL>"<="   {adjust(); return LE;}
<INITIAL>">"    {adjust(); return GT;}
<INITIAL>">="   {adjust(); return GE;}
<INITIAL>"&"    {adjust(); return AND;}
<INITIAL>"|"    {adjust(); return OR;}
<INITIAL>":="   {adjust(); return ASSIGN;}

/* reserved words */
<INITIAL>array    {adjust(); return ARRAY;}
<INITIAL>if       {adjust(); return IF;}
<INITIAL>then     {adjust(); return THEN;}
<INITIAL>else     {adjust(); return ELSE;}
<INITIAL>while    {adjust(); return WHILE;}
<INITIAL>for  	  {adjust(); return FOR;}
<INITIAL>to       {adjust(); return TO;}
<INITIAL>do       {adjust(); return DO;}
<INITIAL>let      {adjust(); return LET;}
<INITIAL>in       {adjust(); return IN;}
<INITIAL>end      {adjust(); return END;}
<INITIAL>of       {adjust(); return OF;}
<INITIAL>break    {adjust(); return BREAK;}
<INITIAL>nil      {adjust(); return NIL;}
<INITIAL>function {adjust(); return FUNCTION;}
<INITIAL>var      {adjust(); return VAR;}
<INITIAL>type     {adjust(); return TYPE;}

/* identifier */
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust(); yylval.sval = String(yytext); return ID;}

/* int */
<INITAL>[0-9]+	{adjust(); yylval.ival = atoi(yytext); return INT;}

/* string literal */
<INITIAL>\"	{adjust(); BEGIN STR;} 
<STR>\"	{adjust(); yylval.sval = String(f_buff.str_buff); BEGIN INITIAL; return STRING;}
<STR>\n	{adjust(); EM_error(EM_tokPos, "unfinished string: newline in string");}
<STR><<EOF>>	{adjust(); EM_error(EM_tokPos, "unfinished string: end of file");}
<STR>\\[0-9]{3}	{
        adjust(); 
        int result; 
        (void)sscanf(yytext[1], "%d", &result);
        if (result > 0xff)
            EM_error(EM_tokPos, "escape sequence too large");
        str_buffer_append((char) result);
    }
<STR>\\[0-9]+   {adjust(); EM_error(EM_tokPos, "bad escape sequence");}
o


<STR>{
	\\n	{adjust(); str_buffer_append('\n');}
	\\t	{adjust(); str_buffer_append('\t');}
	\\\"	{adjust(); str_buffer_append('\"');}
	\\\\	{adjust(); str_buffer_append('\');}
}

/* comment */







