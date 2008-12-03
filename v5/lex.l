%{
/*
   V5: lex.l

   Copyright (C) 2003, 2004 Wojciech Polak.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

   (f)lex lex.l --> lex.yy.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tree.h"
#include "gram.tab.h"

static void stray_character_error (void);
size_t input_line_num;
%}

%x comment

%%
auto                       return AUTO;
break                      return BREAK;
continue                   return CONTINUE;
else                       return ELSE;
function                   return FUNCTION;
global                     return GLOBAL;
if                         return IF;
print                      return PRINT;
return                     return RETURN;
while                      return WHILE;
[a-zA-Z_][a-zA-Z0-9_]*     { yylval.string = strdup(yytext);
                             return ID; }
[0-9]+                     { yylval.number = atoi(yytext);
                             return NUMBER; }
"<="                       return LE_OP;
">="                       return GE_OP;
"<"                        return LT_OP;
">"                        return GT_OP;
"=="                       return EQ_OP;
"!="                       return NE_OP;
"&&"                       return AND_OP;
"||"                       return OR_OP;
[ \t]+                     /* ignore white space */
\n                         input_line_num++;
"(" |
")" |
"{" |
"}" |
"," |
";" |
"+" |
"-" |
"*" |
"/" |
"%" |
"=" |
"!"                        return yytext[0];

"/*"                       BEGIN(comment);
<comment>[^*\n]*           /* ignore */
<comment>"*"+[^*/\n]*      /* ignore */
<comment>\n                input_line_num++;
<comment>"*"+"/"           BEGIN(INITIAL);

"//"[^\n]*\n               input_line_num++;
.                          stray_character_error();
%%

extern int yyerror (const char *s);

int
yywrap (void)
{
  return 1;
}

static void
stray_character_error (void)
{
  char buf[sizeof ("stray character 'xxxx'")];

  if (isprint (yytext[0]) && !isspace (yytext[0])) 
    sprintf (buf, "stray character '%c'", yytext[0]);
  else
    sprintf (buf, "stray character '\\%03o'", yytext[0]);
  yyerror (buf);
}

void
open_file (char *filename)
{
  yyin = fopen (filename, "r");
  if (!yyin) {
    fprintf (stderr, "Cannot open file ");
    perror (filename);
    exit (EXIT_FAILURE);
  }
}

