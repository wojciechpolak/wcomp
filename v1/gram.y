%{
/*
   V1: gram.y

   Copyright (C) 2003 Wojciech Polak.

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

   bison -d -v -t gram.y --> gram.tab.c
                             gram.tab.h
*/

#include <stdio.h>
#include <stdlib.h>

extern int yylex(void);
int yyerror(const char *s);

extern char *yytext;
extern size_t input_line_num;
extern int errcnt;
%}

%token ID NUMBER
%token AUTO BREAK CONTINUE ELSE FUNCTION
%token GLOBAL IF PRINT RETURN WHILE

%nonassoc '='
%left AND_OP
%left OR_OP
%left EQ_OP NE_OP
%left LT_OP GT_OP LE_OP GE_OP
%left '+' '-'
%left '*' '/' '%'
%left UMINUS '!'

%%
input        : /* empty */
             | statement_list
             ;

statement_list
             : statement
             | statement_list statement
             | statement_list error ';'
               {
                  yyerrok;
                  yyclearin;
               }
             ;

statement
             : expression ';'
             | variable_declaration
             | assignment_statement
             | conditional_statement
             | iteration_statement
             | jump_statement
             | print_statement
             | compound_statement
             | function_declaration
             ;

variable_declaration
             : qualifier ID initializer ';'
             ;

qualifier
             : GLOBAL
             | AUTO
             ;

initializer
             : /* empty */
             | '=' expression
             ;

assignment_statement
             : ID '=' expression ';'
             ;

conditional_statement
             : IF '(' expression ')' statement
             | IF '(' expression ')' statement ELSE statement
             ;

iteration_statement
             : WHILE '(' expression ')' statement
             ;

jump_statement
             : RETURN expression ';'
             | BREAK level ';'
             | CONTINUE level ';'
             ;

level
             : /* empty */
             | NUMBER
             ;

print_statement
             : PRINT expression ';'
             ;

compound_statement
             : '{' statement_list '}'
             ;

function_declaration
             : FUNCTION ID '(' identifier_list ')' statement
             ;

identifier_list
             : ID
             | identifier_list ',' ID
             ;

expression_list
             : expression
             | expression_list ',' expression
             ;

expression
             : logical_expr
             ;

basic_expr
             : NUMBER
             | ID
             | function_call
             ;

arithmetic_expr
             : basic_expr
             | arithmetic_expr '+' arithmetic_expr
             | arithmetic_expr '-' arithmetic_expr
             | arithmetic_expr '*' arithmetic_expr
             | arithmetic_expr '/' arithmetic_expr
             | arithmetic_expr '%' arithmetic_expr
             | '-' arithmetic_expr %prec UMINUS
             | '(' arithmetic_expr ')'
             ;

relational_expr
             : arithmetic_expr
             | relational_expr LT_OP relational_expr
             | relational_expr GT_OP relational_expr
             | relational_expr LE_OP relational_expr
             | relational_expr GE_OP relational_expr
             | relational_expr EQ_OP relational_expr
             | relational_expr NE_OP relational_expr
             ;

logical_expr
             : relational_expr
             | logical_expr AND_OP logical_expr
             | logical_expr OR_OP  logical_expr
             | '!' logical_expr
             ;

function_call
             : ID '(' expression_list ')'
             ;
%%

int
yyerror (const char *str)
{
  fprintf (stderr, "%s, line %u, near token '%s'.\n",
	   str, (unsigned) input_line_num, yytext);
  errcnt++;
  return 0;
}

int
parse (void)
{
  char *p = getenv ("YYDEBUG");
  if (p)
    yydebug =  *p - '0';
  input_line_num = 1;
  return yyparse();
}

