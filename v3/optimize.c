/*
   V3: optimize.c

   Copyright (C) 2003, 2004 Wojciech Polak.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "tree.h"
#include "mm.h"
#include "optimize.h"

extern int verbose;
extern int optimize_level;

static void
optimize_pass (int n, NODE *node, traverse_fp *fptab)
{
  if (verbose > 1)
    printf ("\n=== Optimization pass %d ===\n\n", n);

  traverse (node, fptab);
  sweep (mark_free (root));

  if (verbose > 2) {
    printf ("\n=== After optimization pass %d ===\n\n", n);
    print_node (node);
  }
}


/* Pass 1: Operand Sorting */

/* Sort order:
   Variable operands to the right
   Constant operands to the left */

static void
simple_swap (NODE *node)
{
  NODE *p = node->left;
  if (verbose > 1)
    printf ("Swap in node %4.4lu\n", node->node_id);
  node->left = node->right;
  node->right = p;
}

static void
swap_nodes (NODE *node)
{
  switch (node->v.opcode) {
    /* Commutative operations */
  case OPCODE_ADD:
  case OPCODE_MUL:
  case OPCODE_AND:
  case OPCODE_OR:
  case OPCODE_EQ:
  case OPCODE_NE:
    simple_swap(node);
    break;

    /* Anti-commutative operations */
  case OPCODE_SUB:
    simple_swap(node);
    node->left->v.number = - node->left->v.number;
    node->v.opcode = OPCODE_ADD;
    break;

  case OPCODE_DIV:
    break;

    /* Non-associative operators */
  case OPCODE_LT:
  case OPCODE_GT:
  case OPCODE_LE:
  case OPCODE_GE:
    /* nothing to do */
    break;

  default:
    abort ();
  }
}

/*
   Additive and multiplicative transposition
   Algorithm:

   C1 + (C2 +|- V) = (C1 + C2) +|- V
   C1 - (C2 +|- V) = (C1 - C2) -|+ V

   C1 * (C2 *|/ V) = (C1 * C2) *|/ V
   C1 / (C2 *|/ V) = (C1 / C2) /|* V
*/

static enum opcode_type
invert_opcode (enum opcode_type op)
{
  switch (op) {
  case OPCODE_ADD:
    return OPCODE_SUB;
  case OPCODE_SUB:
    return OPCODE_ADD;
  case OPCODE_MUL:
    return OPCODE_DIV;
  case OPCODE_DIV:
    return OPCODE_MUL;
  default:
    abort ();
  }
  /*NOTREACHED*/
  return 0;
}

static void
transpose0 (NODE *node)
{
  NODE *left = node->left;
  NODE *right = node->right;
  enum opcode_type op, rop;

  if (verbose > 1)
    printf ("Transpose, node %4.4lu\n", node->node_id);

  op = node->v.opcode;
  rop = right->v.opcode;

  node->right = right->right;

  right->right = right->left;
  right->left = left;

  node->left = right;

  node->left->v.opcode = op;
  if (op == OPCODE_ADD || op == OPCODE_MUL)
    node->v.opcode = rop;
  else
    node->v.opcode = invert_opcode (rop);
	
  node->left->left = left;
}

static void
transpose (NODE *node)
{
  switch (node->v.opcode) {
  case OPCODE_ADD:
  case OPCODE_SUB:
    switch (node->right->v.opcode) {
    case OPCODE_ADD:
    case OPCODE_SUB:
      transpose0 (node);
    default:
      break;
    }
    break;
  case OPCODE_MUL:
  case OPCODE_DIV:
    switch (node->right->v.opcode) {
    case OPCODE_MUL:
    case OPCODE_DIV:
      transpose0 (node);
    default:
      break;
    }
    break;

  default:
    break;
  }
}

/*
   Algorithm:
   C1 * X / C2 = (C1 / C2) * X
   C1 / X * C2 = (C1 * C2) / X
*/

static void
transpose_left0 (NODE *node)
{
  NODE *left = node->left;
  NODE *right = node->right;
  if (left->type == NODE_BINOP
      && left->v.opcode == OPCODE_MUL
      && left->left->type == NODE_CONST
      && right->type == NODE_CONST)
    {
      NODE *s;
      enum opcode_type op;

      if (verbose > 1)
	printf ("Transpose, node %4.4lu\n", node->node_id);

      op = node->v.opcode;
      node->v.opcode = left->v.opcode;
      left->v.opcode = op;

      s = left->right;
      left->right = node->right;
      node->right = s;
    }
}

static void
transpose_left (NODE *node)
{
  switch (node->v.opcode) {
  case OPCODE_ADD:
  case OPCODE_SUB:
      break;

  case OPCODE_MUL:
  case OPCODE_DIV:
    transpose_left0 (node);
  default:
    break;
  }
}

static void
pass1_binop (NODE *node)
{
  if (node->right->type == NODE_CONST) {
    if (node->left->type == NODE_CONST) {
      return;
    }
    swap_nodes(node);
  }

  switch (node->left->type) {
  case NODE_BINOP:
    transpose_left (node);
    break;
  default:
    break;
  }

  switch (node->right->type) {
  case NODE_BINOP:
    transpose (node);
    break;
  case NODE_UNOP:
    break;
  default:
    break;
  }
}

traverse_fp pass1_fptab[] = {
  NULL,        /* NODE_NOOP */
  NULL,        /* NODE_UNOP */
  pass1_binop, /* NODE_BINOP */
  NULL,        /* NODE_CONST */
  NULL,        /* NODE_VAR */
  NULL,        /* NODE_CALL */
  NULL,        /* NODE_ASGN */
  NULL,        /* NODE_EXPR */
  NULL,        /* NODE_RETURN */
  NULL,        /* NODE_PRINT */
  NULL,        /* NODE_JUMP */
  NULL,        /* NODE_COMPOUND  */
  NULL,        /* NODE_ITERATION */
  NULL,        /* NODE_CONDITION */
  NULL,        /* NODE_VAR_DECL */
  NULL,        /* NODE_FNC_DECL */
};

static void
optimize_pass_1 (NODE *node)
{
  optimize_pass (1, node, pass1_fptab);
}


/* Pass 2: Immediate computations (constant folding) */

static size_t optcnt;

static void
eval_binop_const (NODE *node)
{
  NODE *left = node->left;
  NODE *right = node->right;

  if (verbose > 1)
    printf ("Optimizing node %4.4lu (BINOP)\n", node->node_id);

  switch (node->v.opcode) {
  case OPCODE_ADD:
    node->v.number = left->v.number + right->v.number;
    break;
  case OPCODE_SUB:
    node->v.number = left->v.number - right->v.number;
    break;
  case OPCODE_MUL:
    node->v.number = left->v.number * right->v.number;
    break;
  case OPCODE_DIV:
    if (left->v.number > right->v.number) 
      node->v.number = left->v.number / right->v.number;
    else
      return;
    break;
  case OPCODE_AND:
    node->v.number = left->v.number && right->v.number;
    break;
  case OPCODE_OR:
    node->v.number = left->v.number || right->v.number;
    break;
  case OPCODE_EQ:
    node->v.number = left->v.number == right->v.number;
    break;
  case OPCODE_NE:
    node->v.number = left->v.number != right->v.number;
    break;
  case OPCODE_LT:
    node->v.number = left->v.number < right->v.number;
    break;
  case OPCODE_GT:
    node->v.number = left->v.number > right->v.number;
    break;
  case OPCODE_LE:
    node->v.number = left->v.number <= right->v.number;
    break;
  case OPCODE_GE:
    node->v.number = left->v.number >= right->v.number;
    break;
  case OPCODE_NEG:
  case OPCODE_NOT:
    abort ();
  }
  freenode (left);
  freenode (right);
  node->left = node->right = NULL;
  node->type = NODE_CONST;
  optcnt++;
}

static void
eval_binop_simple (NODE *node)
{
  NODE *left  = node->left;
  NODE *right = node->right;

  if (verbose > 1)
    printf ("Optimizing node %4.4lu (BINOP)\n", node->node_id);

  if (node->v.opcode == OPCODE_MUL)
    {
      /*  x*0 = 0  */
      if (left->v.number == 0)
	{
	  node->type = NODE_CONST;
	  node->v.number = 0;
	}
      /*  x*1 = x  */
      else if (left->v.number == 1)
	{
	  node->type = NODE_VAR;
	  node->v.symbol = right->v.symbol;
	}
    }
  else if (node->v.opcode == OPCODE_ADD)
    {
      /*  x+0 = x  */
      if (left->v.number == 0)
	{
	  node->type = NODE_VAR;
	  node->v.symbol = right->v.symbol;
	}
    }
  freenode (left);
  freenode (right);
  node->left = node->right = NULL;
}

static void
pass2_binop (NODE *node)
{
  NODE *left  = node->left;
  NODE *right = node->right;

  if (left->type == NODE_CONST
      && right->type == NODE_CONST)
    {
      eval_binop_const (node);
    }
  else if (right->type == NODE_VAR && left->type == NODE_CONST)
    {
      if (node->v.opcode == OPCODE_ADD && left->v.number == 0)
	{
	  eval_binop_simple (node);
	}
      else if (node->v.opcode == OPCODE_MUL
	       && (left->v.number == 0 || left->v.number == 1))
	{
	  eval_binop_simple (node);
	}
    }
}

static void
pass2_unop (NODE *node)
{
  NODE *operand = node->left;
  if (operand->type == NODE_CONST) {
    node->type = NODE_CONST;
    node->left = NULL;

    switch (node->v.opcode) {
    case OPCODE_NEG:
      node->v.number = - operand->v.number;
      break;
    case OPCODE_NOT:
      node->v.number = ! operand->v.number;
      break;
    default:
      abort ();
    }
    freenode (operand);
    optcnt++;
  }
}

static void
pass2_asgn (NODE *node)
{
  if (node->v.asgn.expr->v.expr->type == NODE_VAR
      && node->v.asgn.symbol == node->v.asgn.expr->v.expr->v.symbol)
    {
      if (verbose > 1)
	printf ("Optimizing node %4.4lu (ASGN)\n", node->node_id);

      freenode (node->v.asgn.expr);
      node->v.asgn.expr = NULL;
      node->type = NODE_NOOP;
    }
}

traverse_fp pass2_fptab[] = {
  NULL,        /* NODE_NOOP */
  pass2_unop,  /* NODE_UNOP */
  pass2_binop, /* NODE_BINOP */
  NULL,        /* NODE_CONST */
  NULL,        /* NODE_VAR */
  NULL,        /* NODE_CALL */
  pass2_asgn,  /* NODE_ASGN */
  NULL,        /* NODE_EXPR */
  NULL,        /* NODE_RETURN */
  NULL,        /* NODE_PRINT */
  NULL,        /* NODE_JUMP */
  NULL,        /* NODE_COMPOUND  */
  NULL,        /* NODE_ITERATION */
  NULL,        /* NODE_CONDITION */
  NULL,        /* NODE_VAR_DECL */
  NULL,        /* NODE_FNC_DECL */
};

static void
optimize_pass_2 (NODE *node)
{
  optimize_pass (2, node, pass2_fptab);
}


/* Pass 3: Substitution of constant variables (constant propagation) */

#define VAR_IS_CONST(s) (s && s->v.var->entry_point && \
                         s->v.var->entry_point->v.expr->type == \
                         NODE_CONST)

static void
pass3_var (NODE *node)
{
  SYMBOL *s = node->v.symbol;

  if (VAR_IS_CONST (s))
    {
      if (verbose > 1)
	printf ("Optimizing node %4.4lu (VAR)\n", node->node_id);

      node->v.expr = NULL;
      node->v.number = s->v.var->entry_point->v.expr->v.number;
      node->type = NODE_CONST;
      optcnt++;
    }
}

static void
pass3_var_decl (NODE *node)
{
  node->v.vardecl.symbol->v.var->entry_point = node->v.vardecl.expr;
}

static void
pass3_asgn (NODE *node)
{
  node->v.asgn.symbol->v.var->entry_point = node->v.asgn.expr;
}

traverse_fp pass3_fptab[] = {
  NULL,           /* NODE_NOOP */
  NULL,           /* NODE_UNOP */
  NULL,           /* NODE_BINOP */
  NULL,           /* NODE_CONST */
  pass3_var,      /* NODE_VAR */
  NULL,           /* NODE_CALL */
  pass3_asgn,     /* NODE_ASGN */
  NULL,           /* NODE_EXPR */
  NULL,           /* NODE_RETURN */
  NULL,           /* NODE_PRINT */
  NULL,           /* NODE_JUMP */
  NULL,           /* NODE_COMPOUND  */
  NULL,           /* NODE_ITERATION */
  NULL,           /* NODE_CONDITION */
  pass3_var_decl, /* NODE_VAR_DECL */
  NULL,           /* NODE_FNC_DECL */
};

static void
optimize_pass_3 (NODE *node)
{
  optimize_pass (3, node, pass3_fptab);
}


/* Entry point */
void
optimize_tree (NODE *root)
{
  if (optimize_level == 0)
    return;
  
  do {
    optimize_pass_1 (root);
    optcnt = 0;
    optimize_pass_2 (root);
    optimize_pass_3 (root);
  } while (optcnt);
}

