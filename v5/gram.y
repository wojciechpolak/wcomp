%{
/*
   V5: gram.y

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

   bison -d -v -t gram.y --> gram.tab.c
                             gram.tab.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "symbol.h"
#include "tree.h"

extern int yylex (void);
int yyerror (const char *);
void parse_error (const char *, ...);

extern char *yytext;
extern size_t input_line_num;
extern int errcnt;
%}

%union
{
  char *string;
  long number;
  enum qualifier_type qualifier;
  NODE *node;
  SYMBOL *symbol;
  ARGLIST *arglist;
  SYMLIST *symlist;
  struct {
    NODE *head;
    NODE *tail;
  } nodelist;
}

%type <node> statement
%type <node> expression
%type <node> variable_declaration
%type <node> assignment_statement
%type <node> conditional_statement
%type <node> iteration_statement
%type <node> jump_statement
%type <node> print_statement
%type <node> compound_statement
%type <node> function_declaration
%type <node> function_call
%type <node> identifier
%type <node> basic_expr
%type <node> arithmetic_expr
%type <node> relational_expr
%type <node> logical_expr
%type <node> initializer
%type <nodelist> statement_list
%type <symbol> fundecl_header
%type <arglist> expression_list
%type <symlist> identifier_list
%type <qualifier> qualifier
%type <number> level

%token <string> ID
%token <number> NUMBER
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
               {
                  root = $1.head;
               }
             ;

statement_list
             : statement
               {
                  $1->right = $1->left = NULL;
                  $$.head = $$.tail = $1;
               }
             | statement_list statement
               {
                  $2->left = $2->right = NULL;
                  $1.tail->right = $2;
                  $1.tail = $2;
                  $$ = $1;
               }
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
               {
                  SYMBOL *s = putsym (&symbol_variables, $2, SYMBOL_VAR);
                  if (s)
                    s->v.var->qualifier = $1;

                  $$ = addnode (NODE_VAR_DECL);
                  $$->v.vardecl.symbol = s;
                  $$->v.vardecl.expr = $3;
               }
             ;

qualifier
             : GLOBAL
               {
                  $$ = QUA_GLOBAL;
               }
             | AUTO
               {
                  $$ = QUA_AUTO;
               }
             ;

initializer
             : /* empty */
               {
                  $$ = NULL;
               }
             | '=' expression
               {
                  $$ = $2;
               }
             ;

assignment_statement
             : identifier '=' expression ';'
               {
                 $$ = addnode (NODE_ASGN);
                 $$->v.asgn.symbol = $1->v.symbol;
                 $$->v.asgn.expr = $3;
               }
             ;

conditional_statement
             : IF '(' expression ')' statement
               {
                  $$ = addnode (NODE_CONDITION);
                  $$->v.condition.cond = $3;
                  $$->v.condition.iftrue_stmt = $5;
               }
             | IF '(' expression ')' statement ELSE statement
               {
                  $$ = addnode (NODE_CONDITION);
                  $$->v.condition.cond = $3;
                  $$->v.condition.iftrue_stmt  = $5;
                  $$->v.condition.iffalse_stmt = $7;
               }
             ;

iteration_statement
             : WHILE '(' expression ')' statement
               {
                  $$ = addnode (NODE_ITERATION);
                  $$->v.iteration.cond = $3;
                  $$->v.iteration.stmt = $5;
               }
             ;

jump_statement
             : RETURN expression ';'
               {
                  $$ = addnode (NODE_RETURN);
                  $$->v.expr = $2;
               }
             | BREAK level ';'
               {
                  $$ = addnode (NODE_JUMP);
                  $$->v.jump.type  = JUMP_BREAK;
                  $$->v.jump.level = $2;
               }
             | CONTINUE level ';'
               {
                  $$ = addnode (NODE_JUMP);
                  $$->v.jump.type  = JUMP_CONTINUE;
                  $$->v.jump.level = $2;
               }
             ;

level
             : /* empty */
               {
                  $$ = 0;
               }
             | NUMBER
             ;

print_statement
             : PRINT expression ';'
               {
                  $$ = addnode (NODE_PRINT);
                  $$->v.expr = $2;
               }
             ;

compound_statement
             : lbrace statement_list rbrace
               {
                  $$ = addnode (NODE_COMPOUND);
                  $$->v.expr = $2.head;
                  delsym_level (&symbol_variables, nesting_level+1);
               }
             ;

lbrace
             : '{'
               {
                  nesting_level++;
               }
             ;

rbrace
             : '}'
               {
                  nesting_level--;
                  if (nesting_level < 0)
                    parse_error ("nesting error!");
               }
             ;

function_declaration
             : fundecl_header statement
               {
                  $$ = addnode (NODE_FNC_DECL);
                  $1->v.fnc->entry_point = $2;
                  $$->v.fncdecl.symbol = $1;
                  $$->v.fncdecl.stmt = $2;
               }
             ;

fundecl_header
             : FUNCTION ID '(' identifier_list ')'
               {
                  SYMLIST *np;
		  size_t nparam;

                  /* Count the number of parameters */
                  for (np = $4, nparam = 0; np; np = np->next)
                     nparam++;

                  $$ = putsym (&symbol_functions, $2, SYMBOL_FNC);
		  $$->v.fnc->nparam = nparam;
                  $$->v.fnc->param = $4;
               }
             ;

identifier_list
             : ID
               {
                  SYMBOL *s = putsym (&symbol_variables, $1, SYMBOL_VAR);
                  if (s) {
                    s->v.var->level++;
                    s->v.var->qualifier = QUA_PARAMETER;
                  }
                  $$ = make_symlist (s, NULL);
               }
             | identifier_list ',' ID
               {
                  SYMBOL *s = putsym (&symbol_variables, $3, SYMBOL_VAR);
                  if (s) {
                    s->v.var->level++;
                    s->v.var->qualifier = QUA_PARAMETER;
                  }
                  $$ = make_symlist (s, $1);
               }
             ;

identifier
             : ID
               {
                  $$ = addnode (NODE_VAR);
                  $$->v.symbol = getsym (symbol_variables, $1);
                  if (!$$->v.symbol)
                    parse_error ("Undefined variable `%s'", $1);
               }
             ;

expression_list
             : expression
               {
                  $$ = make_arglist ($1, NULL);
               }
             | expression_list ',' expression
               {
                  $$ = make_arglist ($3, $1);
               }
             ;

expression
             : logical_expr
               {
                  $$ = addnode (NODE_EXPR);
                  $$->v.expr = $1;
               }
             ;

basic_expr
             : NUMBER
               {
                  $$ = addnode (NODE_CONST);
                  $$->v.number = $1;
               }
             | identifier
             | function_call
             ;

arithmetic_expr
             : basic_expr
             | arithmetic_expr '+' arithmetic_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_ADD;
               }
             | arithmetic_expr '-' arithmetic_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_SUB;
               }
             | arithmetic_expr '*' arithmetic_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_MUL;
               }
             | arithmetic_expr '/' arithmetic_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_DIV;
               }
             | arithmetic_expr '%' arithmetic_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
               }
             | '-' arithmetic_expr %prec UMINUS
               {
                  $$ = addnode (NODE_UNOP);
                  $$->left = $2;
                  $$->v.opcode = OPCODE_NEG;
               }
             | '(' arithmetic_expr ')'
               {
                  $$ = $2;
               }
             ;

relational_expr
             : arithmetic_expr
             | relational_expr LT_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_LT;
               }
             | relational_expr GT_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_GT;
               }
             | relational_expr LE_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_LE;
               }
             | relational_expr GE_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_GE;
               }
             | relational_expr EQ_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_EQ;
               }
             | relational_expr NE_OP relational_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_NE;
               }
             ;

logical_expr
             : relational_expr
             | logical_expr AND_OP logical_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_AND;
               }
             | logical_expr OR_OP logical_expr
               {
                  $$ = addnode (NODE_BINOP);
                  $$->left = $1;
                  $$->right = $3;
                  $$->v.opcode = OPCODE_OR;
               }
             | '!' logical_expr
               {
                  $$ = addnode (NODE_UNOP);
                  $$->left = $2;
                  $$->v.opcode = OPCODE_NOT;
               }
             ;

function_call
             : ID '(' expression_list ')'
               {
                  SYMBOL *s;
                  ARGLIST *np;
                  size_t nparam;

                  s = getsym (symbol_functions, $1);
                  if (!s)
                    {
                      parse_error ("Function `%s' is not defined", $1);
                      $$ = NULL;
                    }
                  else
                    {
                      /* Count the number of arguments */
                      for (np = $3, nparam = 0; np; np = np->next)
                         nparam++;

                      /* Check if the number of args is OK */
                      if (s->v.fnc->nparam < nparam)
                        parse_error ("Too many arguments in call to `%s'", $1);
                      else if (s->v.fnc->nparam > nparam)
                        parse_error ("Too few arguments in call to `%s'", $1);

                      /* Create a new node */
                      $$ = addnode (NODE_CALL);
                      $$->v.funcall.symbol = s;
                      $$->v.funcall.args = $3;
                    }
               }
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

void
parse_error (const char *fmt, ...)
{
  va_list arglist;
  char buf[256];

  va_start (arglist, fmt);
  vsnprintf (buf, 255, fmt, arglist);
  va_end (arglist);

  fprintf (stderr, "%s, line %u.",
	   buf, (unsigned) input_line_num);
  fputc ('\n', stderr);
  errcnt++;
}

int
parse (void)
{
  char *p = getenv ("YYDEBUG");
  if (p)
    yydebug =  *p - '0';
  input_line_num = 1;
  return yyparse ();
}

