/*
   V2: tree.c

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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

NODE *root; /* the root of a parse tree */
NODE *memory_pool;

static unsigned int last_node_id;

NODE *
addnode (enum node_type type)
{
  NODE *new;

  new = (NODE *)malloc (sizeof(NODE));
  if (!new)
    exit (EXIT_FAILURE);
  memset (new, 0, sizeof(NODE));

  new->node_id = ++last_node_id;
  new->type    = type;
  new->left    = NULL;
  new->right   = NULL;

  new->memory_link = memory_pool;
  memory_pool = new;
  return new;
}

void
free_all_nodes (void)
{
  NODE *p, *next;

  for (p = memory_pool; p; p = next)
    {
      next = p->memory_link;
      free (p);
    }
  memory_pool = NULL;
}

ARGLIST *
make_arglist (NODE *node, ARGLIST *next)
{
  ARGLIST *x;

  x = (ARGLIST *)malloc (sizeof(ARGLIST));
  if (!x)
    exit (EXIT_FAILURE);

  x->node = node;
  x->next = next;
  return x;
}

/*
  All the functions below are designed to create
  a visible form of the parse tree.
*/

unsigned int
get_last_node_id (void)
{
  return last_node_id;
}

static void
print_node_id (NODE *node)
{
  if (node)
    printf (" %4.4lu", node->node_id);
  else
    printf (" %4.4s", "NIL");
}

static void
print_binop (NODE *node)
{
  printf ("\t NODE_BINOP");
  printf ("\t opcode = ");

  switch (node->v.opcode) {
  case OPCODE_ADD:
    printf ("OPCODE_ADD");
    break;
  case OPCODE_SUB:
    printf ("OPCODE_SUB");
    break;
  case OPCODE_MUL:
    printf ("OPCODE_MUL");
    break;
  case OPCODE_DIV:
    printf ("OPCODE_DIV");
    break;
  case OPCODE_NEG:
    printf ("OPCODE_NEG");
    break;
  case OPCODE_AND:
    printf ("OPCODE_AND");
    break;
  case OPCODE_NOT:
    printf ("OPCODE_NOT");
    break;
  case OPCODE_OR:
    printf ("OPCODE_OR");
    break;
  case OPCODE_EQ:
    printf ("OPCODE_EQ");
    break;
  case OPCODE_NE:
    printf ("OPCODE_NE");
    break;
  case OPCODE_LT:
    printf ("OPCODE_LT");
    break;
  case OPCODE_GT:
    printf ("OPCODE_GT");
    break;
  case OPCODE_LE:
    printf ("OPCODE_LE");
    break;
  case OPCODE_GE:
    printf ("OPCODE_GT");
    break;
  default:
    printf ("UNKNOWN OPCODE");
  }
  fputc ('\n', stdout);
}

static void
print_unop (NODE *node)
{
  printf ("\t NODE_UNOP");
  printf ("\t opcode = %d", node->v.opcode);
  fputc ('\n', stdout);
}

static void
print_const (NODE *node)
{
  printf ("\t NODE_CONST");
  printf ("\t number = %ld", node->v.number);
  fputc ('\n', stdout);
}

static void
print_var (NODE *node)
{
  printf ("\t NODE_VAR");
  printf ("\t var = %s", node->v.symbol->name);
  fputc ('\n', stdout);
}

static void
print_call (NODE *node)
{
  ARGLIST *ptr;
	
  printf ("\t NODE_CALL");
  printf ("\t node =");
  print_node_id (node->v.funcall.symbol->v.fnc->entry_point);
  printf (", args = ");
  for (ptr = node->v.funcall.args; ptr; ptr = ptr->next)
    {
      printf ("%4.4lu ", ptr->node->node_id);
    }
  fputc ('\n', stdout);

  for (ptr = node->v.funcall.args; ptr; ptr = ptr->next)
    {
      print_node (ptr->node);
    }
}

static void
print_asgn (NODE *node)
{
  printf ("\t NODE_ASGN");
  printf ("\t var = %s",
	  node->v.asgn.symbol ? node->v.asgn.symbol->name : "NIL");
  printf (", expr =");
  print_node_id (node->v.asgn.expr);
  fputc ('\n', stdout);
  print_node (node->v.asgn.expr);
}

static void
print_expr (NODE *node)
{
  printf ("\t NODE_EXPR");
  printf ("\t expr =");
  print_node_id (node->v.expr);
  fputc ('\n', stdout);
  print_node (node->v.expr);
}

static void
print_return (NODE *node)
{
  printf ("\t NODE_RETURN");
  printf ("\t expr =");
  print_node_id (node->v.expr);
  fputc ('\n', stdout);
  print_node (node->v.expr);
}

static void
print_print (NODE *node)
{
  printf ("\t NODE_PRINT");
  printf ("\t expr =");
  print_node_id (node->v.expr);
  fputc ('\n', stdout);
  print_node (node->v.expr);
}

static void
print_jump (NODE *node)
{
  printf ("\t NODE_JUMP");
  printf ("\t type = ");

  switch (node->v.jump.type) {
  case JUMP_BREAK:
    printf ("BREAK");
    break;
  case JUMP_CONTINUE:
    printf ("CONTINUE");
    break;
  default:
    printf ("UNKNOWN JUMP");
  }
  printf (" level = %u", node->v.jump.level);
  fputc ('\n', stdout);
}

static void
print_compound (NODE *node)
{
  printf ("\t NODE_COMPOUND");
  printf ("\t expr =");
  print_node_id (node->v.expr);
  fputc ('\n', stdout);
  print_node (node->v.expr);
}

static void
print_iteration (NODE *node)
{
  printf ("\t NODE_ITERATION");
  printf ("\t cond =");
  print_node_id (node->v.iteration.cond);
  printf(", stmt =");
  print_node_id (node->v.iteration.stmt);
  fputc ('\n', stdout);
  print_node (node->v.iteration.cond);
  print_node (node->v.iteration.stmt);
}

static void
print_condition (NODE *node)
{
  printf ("\t NODE_CONDITION");
  printf ("\t cond =");
  print_node_id (node->v.condition.cond);
  printf (", iftrue =");
  print_node_id (node->v.condition.iftrue_stmt);
  printf (", iffalse =");
  print_node_id (node->v.condition.iffalse_stmt);
  fputc ('\n', stdout);
  print_node (node->v.condition.cond);
  print_node (node->v.condition.iftrue_stmt);
  print_node (node->v.condition.iffalse_stmt);
}

static void
print_var_decl (NODE *node)
{
  printf ("\t NODE_VAR_DECL");
  printf ("\t name = %s",
	  node->v.vardecl.symbol ? node->v.vardecl.symbol->name : "NIL");
  printf (", expr =");
  print_node_id (node->v.vardecl.expr);
  fputc ('\n', stdout);
  print_node (node->v.vardecl.expr);
}

static void
print_fnc_decl (NODE *node)
{
  printf ("\t NODE_FNC_DECL");
  printf ("\t name = %s",
	  node->v.fncdecl.symbol ? node->v.fncdecl.symbol->name : "NIL");
  printf (", stmt =");
  print_node_id (node->v.fncdecl.stmt);
  fputc ('\n', stdout);
  print_node (node->v.fncdecl.stmt);
}

/*
 */

void
print_node (NODE *node)
{
 tail_recurse:
  if (!node)
    return;

  print_node_id (node);
  print_node_id (node->left);
  print_node_id (node->right);

  switch (node->type) {
  case NODE_BINOP:
    print_binop (node);
    break;
  case NODE_UNOP:
    print_unop (node);
    break;
  case NODE_CONST:
    print_const (node);
    break;
  case NODE_VAR:
    print_var (node);
    break;
  case NODE_CALL:
    print_call (node);
    break;
  case NODE_ASGN:
    print_asgn (node);
    break;
  case NODE_EXPR:
    print_expr (node);
    break;
  case NODE_RETURN:
    print_return (node);
    break;
  case NODE_PRINT:
    print_print (node);
    break;
  case NODE_JUMP:
    print_jump (node);
    break;
  case NODE_COMPOUND:
    print_compound (node);
    break;
  case NODE_ITERATION:
    print_iteration (node);
    break;
  case NODE_CONDITION:
    print_condition (node);
    break;
  case NODE_VAR_DECL:
    print_var_decl (node);
    break;
  case NODE_FNC_DECL:
    print_fnc_decl (node);
    break;
  default:
    abort();
  }

  if (!node->right) {
    node = node->left;
    goto tail_recurse;
  }
  if (!node->left) {
    node = node->right;
    goto tail_recurse;
  }

  print_node (node->right);
  print_node (node->left);
}

