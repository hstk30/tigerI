%{
#include <string.h>
#include "util.h"
#include "absyn.h"
#include "y.tab.h"
#include "errormsg.h"

#define INIT_CAPACITY 32
#define EXTRA_BUFF 32

int charPos = 1;
static int comment_lev = 0;

struct FBuffer {
    string str_buf;
    size_t len;
    size_t capacity;
} f_buf;

static void
init_str_buffer() {
    string str_buf = checked_malloc(INIT_CAPACITY);

    f_buf.str_buf = str_buf;
    f_buf.len = 0;
    f_buf.capacity = INIT_CAPACITY;
    str_buf[0] = '\0';
}

static void
str_buffer_append(char c) {
    if (f_buf.len + 1 > f_buf.capacity - 1) {
        string new_buf = checked_malloc(f_buf.capacity + EXTRA_BUFF);
        strcpy(new_buf, f_buf.str_buf);
        free(f_buf.str_buf);

        f_buf.str_buf = new_buf;
        f_buf.capacity = f_buf.capacity + EXTRA_BUFF;
    } else {
        f_buf.str_buf[f_buf.len ++] = c;
        f_buf.str_buf[f_buf.len] = '\0';
    }
}

int yywrap(void) {
    if (comment_lev != 0)
        EM_error(charPos, "End-Of-File Unclosed comment!");
    charPos = 1;
    comment_lev = 0;
    return 1;
}

void adjust(void) {
    EM_tokPos = charPos;
    charPos += yyleng;
}

static int
to_hex(char c) {
    int hex;
    if (('0' <= c) && (c <= '9'))
        hex = c - '0';
    else if (('a' <= c) && (c <= 'f'))
        hex = c - 'a' + 10;
    else if (('A' <= c) && (c <= 'F'))
        hex = c - 'A' + 10;
    else {
        return -1;
    }
    return hex;
}

static int 
to_oct(char c) {
    int oct;
    if (('0' <= c) && (c <= '7')) {
        oct = c - '0';
        return oct;
    }
    else 
        return -1;
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
                            if ((uh == -1) || (lh == -1)) {
                                EM_error(charPos, "In \"\\xnum\" num must composed of exactly 2 hexadecimal characters.");
                                err = 1;
                            }
                            escape_ch = uh * 16 + lh;
                            assert(escape_ch < 256);
                        } else if (('0' <= ch) && (ch <= '3')) {
                            int ho = ch - '0';
                            int mo = to_oct(input());
                            int lo = to_oct(input());
                            if ((mo == -1) || (lo == -1)) {
                                EM_error(charPos, "In \"\\num\" num must between 0 and 255 in decimal (0 and 377 in octal)");
                                err = 1;
                            }
                            escape_ch = ho * 64 + mo * 8 + lo;
                            assert(escape_ch < 256);
                        } else if (ch > '3') {
                                EM_error(charPos, "In \"\\num\" num must between 0 and 255 in decimal (0 and 377 in octal)");
                                err = 1;
                        } else {
                            EM_error(charPos, "Other escape: \\%c is not support", ch); err = 1;
                        }
                        if (err == 0)
                            str_buffer_append(escape_ch);
                    }
				}
                break;
            case '\"':
                yylval.sval = String(f_buf.str_buf); 
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
    yylval.sval = String(f_buf.str_buf); 
    return STRING;
    }

"/*"    {
    adjust(); 
    int c;
    comment_lev ++;
    while ((c = input()) != EOF) {
        charPos ++;
        switch (c) {
            case '\n': 
                EM_newline(); break;
            case '/': 
                if ((c = input()) == '*') {
                    charPos ++;
                    comment_lev ++;
                } else {
                    unput(c);
                }
                break;
            case '*': 
                while ((c = input()) == '*') charPos ++;
                if (c  == '/')
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
    }

