%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

#define INIT_CAPACITY 32
#define EXTRA_BUFF 32

int charPos = 1;
static int comment_lev = 0;

struct FBuffer {
    string str_buff;
    size_t len;
    size_t capacity;
} f_buff;

static void
init_str_buffer() {
    string str_buff = checked_malloc(INIT_CAPACITY);

    f_buff.str_buff = str_buff;
    f_buff.len = 0;
    f_buff.capacity = INIT_CAPACITY;
    str_buff[0] = '\0';
}

static void
str_buffer_append(char c) {
    if (f_buff.len + 1 > f_buff.capacity - 1) {
        string new_buff = checked_malloc(f_buff.capacity + EXTRA_BUFF);
        strcpy(new_buff, f_buff.str_buff);
        free(f_buff.str_buff);

        f_buff.str_buff = new_buff;
        f_buff.capacity = f_buff.capacity + EXTRA_BUFF;
    } else {
        f_buff.str_buff[f_buff.len ++] = c;
        f_buff.str_buff[f_buff.len] = '\0';
    }
}

int yywrap(void) {
    charPos=1;
    return 1;
}

void adjust(void) {
    EM_tokPos = charPos;
    charPos += yyleng;
}

static int
to_hex(char c) {
    int hex;
    if (('0' <= c) && (c <= '7'))
        hex = c - '0';
    else if (('a' <= c) && (c <= 'f'))
        hex = c - 'a';
    else if (('A' <= c) && (c <= 'F'))
        hex = c - 'A';
    else {
        EM_error(charPos, "In \"\\xnum\" num must composed of exactly 2 hexadecimal characters.");
        return -1;
    }
    return hex;
}

static int 
to_oct(char c) {
    int oct;
    if (('0' <= c) && (c <= '7'))
        oct = c - '0';
    else{
        EM_error(charPos, "In \"\\num\" num must between 0 and 255 in decimal (0 and 377 in octal)");
        return -1;
    }
    return oct;
}

%}

%%

[ \t\r]	{adjust(); continue;}

\n       {adjust(); EM_newline(); continue;}

","	  {adjust(); return COMMA;}
":"    {adjust(); return COLON;}
";"    {adjust(); return SEMICOLON;}
"("    {adjust(); return LPAREN;}
")"    {adjust(); return RPAREN;}
"["    {adjust(); return LBRACK;}
"]"    {adjust(); return RBRACK;}
"{"    {adjust(); return LBRACE;}
"}"    {adjust(); return RBRACE;}
"."    {adjust(); return DOT;}
"+"    {adjust(); return PLUS;}
"-"    {adjust(); return MINUS;}
"*"    {adjust(); return TIMES;}
"/"    {adjust(); return DIVIDE;}
"="    {adjust(); return EQ;}
"<>"   {adjust(); return NEQ;}
"<"    {adjust(); return LT;}
"<="   {adjust(); return LE;}
">"    {adjust(); return GT;}
">="   {adjust(); return GE;}
"&"    {adjust(); return AND;}
"|"    {adjust(); return OR;}
":="   {adjust(); return ASSIGN;}

array    {adjust(); return ARRAY;}
if       {adjust(); return IF;}
then     {adjust(); return THEN;}
else     {adjust(); return ELSE;}
while    {adjust(); return WHILE;}
for  	  {adjust(); return FOR;}
to       {adjust(); return TO;}
do       {adjust(); return DO;}
let      {adjust(); return LET;}
in       {adjust(); return IN;}
end      {adjust(); return END;}
of       {adjust(); return OF;}
break    {adjust(); return BREAK;}
nil      {adjust(); return NIL;}
function {adjust(); return FUNCTION;}
var      {adjust(); return VAR;}
type     {adjust(); return TYPE;}

[a-zA-Z][a-zA-Z0-9_]* {adjust(); yylval.sval = String(yytext); return ID;}

[0-9]+	{adjust(); yylval.ival = atoi(yytext); return INT;}

\"  {
    adjust(); 
    int ch;
    int err = 0;
    init_str_buffer();

	while ((ch = input()) != EOF) {
        charPos ++;
		switch (ch) {
			case '\n':
                EM_newline();
                str_buffer_append('\n');
				break;  
			case '\\':
                ch = input();
                charPos ++;
				switch (ch) {
					case 'a': str_buffer_append('\a'); break;
					case 'b': str_buffer_append('\b'); break;
					case 'f': str_buffer_append('\f'); break;
					case 'n': str_buffer_append('\n'); break;
					case 'r': str_buffer_append('\r'); break;
					case 't': str_buffer_append('\t'); break;
					case 'v': str_buffer_append('\v'); break;
					case '\\': str_buffer_append('\\'); break;
					case '\"': str_buffer_append('\"'); break;
					case '\'': str_buffer_append('\''); break;
					default: {
                        int escape_ch;
                        if (ch == 'x') {
                            int uh = to_hex(input());
                            int lh = to_hex(input());
                            charPos +=2;
                            if ((uh == -1) || (lh == -1)) err = 1;
                            escape_ch = uh * 16 + lh;
                        } else if (('0' <= ch) && (ch <= '3')) {
                            int ho = ch - '0';
                            int mo = to_oct(input());
                            int lo = to_oct(input());
                            if ((mo == -1) || (lo == 01)) err = 1;
                            escape_ch = ho * 64 + mo * 8 + lo;
                        } else {
                            EM_error(charPos, "\\%c is not support", ch); err = 1;
                        }
                        if (err == 0)
                            str_buffer_append(escape_ch);
                    }
				}
                break;
            case '\"':
                yylval.sval = String(f_buff.str_buff); 
                return STRING;
			default:
                str_buffer_append(ch);
		}
        if (err == 1) break;
	}
    if (err) {
        while ((ch = input()) != EOF) {
            charPos ++;
            if (ch == '\"')
                break;
        }
    }
    if (ch == EOF) {
        EM_error(charPos, "Unclosed string!");
        yyterminate();
    }
    yylval.sval = String(f_buff.str_buff); 
    return STRING;
    }

"/*"    {
    adjust(); 
    int c;
    comment_lev ++;
    while((c = input()) != EOF) {
        charPos ++;
        switch (c) {
            case '\n': 
                EM_newline(); break;
            case '/': 
                if ((c = input()) == '*') 
                    comment_lev ++;
                else
                    unput(c);
                break;
            case '*': 
                if ((c = input()) == '/')
                    comment_lev --;
                else
                    unput(c);
                break;
            default: break;
        }
        if (comment_lev == 0) {
            break;
        }
    } 
    if (comment_lev != 0)
        EM_error(charPos, "Unclosed comment!");
    if (c == EOF) {
        EM_error(charPos, "End-Of-File Unclosed comment!");
        yyterminate();
    }
    continue;
    }

