/*
   V4: optimize.c

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

#include "tree.h"
#include "mm.h"
#include "optimize.h"

/* Generalized interface */
typedef void (*optimize_fp)(NODE *);

/* Prototypes */
static void optimize (NODE *, optimize_fp *);
static void pass2_binop (NODE *);
static void run_optimize_node (NODE *, optimize_fp *);
static NODE *mark_free (NODE *);

extern int verbose;
extern int optimize_level;

static void
optimize_funcall (NODE *node, optimize_fp *opttab)
{
  ARGLIST *ptr;
  for (ptr = node->v.funcall.args; ptr; ptr = ptr->next) 
    run_optimize_node (ptr->node, opttab);
}

/* General routines */
static void
run_optimize_node (NODE *node, optimize_fp *opttab)
{
  if (!node)
    return;

  run_optimize_node (node->left, opttab);
  run_optimize_node (node->right, opttab);

  switch (node->type) {
  case NODE_CALL:
    optimize_funcall (node, opttab);
    break;
  case NODE_BINOP:
  case NODE_UNOP:
  case NODE_CONST:
  case NODE_VAR:
  case NODE_NOOP:
    break;
  case NODE_EXPR:
    run_optimize_node (node->v.expr, opttab);
    break;
  default:
    abort();
  }
  if (opttab[node->type])
    opttab[node->type](node);
}

static void
run_optimize_stmt (NODE *node, optimize_fp *opttab)
{
  switch (node->type) {
  case NODE_CALL:
    optimize_funcall(node, opttab);
    break; 
  case NODE_ASGN:
    run_optimize_node (node->v.asgn.expr, opttab);
    break;
  case NODE_EXPR:
  case NODE_RETURN:
  case NODE_PRINT:
    run_optimize_node (node->v.expr, opttab);
    break;
  case NODE_JUMP:
    break;
  case NODE_COMPOUND:
    optimize (node->v.expr, opttab);	  
    break;
  case NODE_ITERATION:
    run_optimize_node (node->v.iteration.cond, opttab);
    optimize (node->v.iteration.stmt, opttab);
    break;
  case NODE_CONDITION:
    run_optimize_node (node->v.condition.cond, opttab);
    optimize (node->v.condition.iftrue_stmt, opttab);
    optimize (node->v.condition.iffalse_stmt, opttab);
    break;
  case NODE_VAR_DECL:
    run_optimize_node (node->v.vardecl.expr, opttab);
    break;
  case NODE_FNC_DECL:
    optimize (node->v.fncdecl.stmt, opttab);
    break;
  case NODE_NOOP:
    break;
  default:
    abort();
  }

  if (opttab[node->type]) 
    opttab[node->type](node);
}

static void
optimize (NODE *node, optimize_fp *opttab)
{
  for (; node; node = node->right)
    run_optimize_stmt (node, opttab);
}

static void
optimize_pass (int n, NODE *node, optimize_fp *opttab)
{
  if (verbose)
    printf ("\n=== Optimization pass %d ===\n\n", n);

  optimize (node, opttab);
  sweep (mark_free (root));

  if (verbose) {
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
  if (verbose)
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
    abort();
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
    abort();
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

  if (verbose)
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

      if (verbose)
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

optimize_fp pass1_opttab[] = {
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
  optimize_pass (1, node, pass1_opttab);
}


/* Pass 2: Immediate computations */

static size_t optcnt;

static void
eval_binop_const (NODE *node)
{
  NODE *left = node->left;
  NODE *right = node->right;

  if (verbose)
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
    abort();
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

  if (verbose)
    printf ("Optimizing node %4.4lu (BINOP)\n", node->node_id);

  if (node->v.opcode == OPCODE_MUL)
    {
      if (left->v.number == 0)
	{
	  /*  0*x = 0  */
	  node->type = NODE_CONST;
	  node->v.number = 0;
	}
      else if (left->v.number == 1)
	{
	  /*  1*x = x  */
	  node->type = NODE_VAR;
	  node->v.symbol = right->v.symbol;
	}
    }
  else if (node->v.opcode == OPCODE_ADD)
    {
      if (left->v.number == 0)
	{
	  /*  0+x = x  */
	  node->type = NODE_VAR;
	  node->v.symbol = right->v.symbol;
	}
    }

  freenode (left);
  freenode (right);
  node->left = node->right = NULL;
}

static void
eval_binop_simple_logic (NODE *node)
{
  NODE *right = node->right;

  if (verbose)
    printf ("Optimizing node %4.4lu (BINOP)\n", node->node_id);

  if (node->v.opcode == OPCODE_AND)
    {
      /*  1 && BINOP = BINOP  */
      node->type  = right->type;
      node->left  = right->left;
      node->right = right->right;
      node->v.opcode = right->v.opcode;
    }
  else if (node->v.opcode == OPCODE_OR)
    {
      /*  1 || BINOP = 1  */
      node->type  = NODE_CONST;
      node->left  = NULL;
      node->right = NULL;
      node->v.number = 1;
    }
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
  else if (left->type == NODE_CONST
	   && right->type == NODE_VAR)
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
  else if (left->type == NODE_CONST
	   && right->type == NODE_BINOP)
    {
      if ((node->v.opcode == OPCODE_AND || node->v.opcode == OPCODE_OR)
	  && left->v.number != 0)
	{
	  eval_binop_simple_logic (node);
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
      abort();
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
      if (verbose)
	printf ("Optimizing node %4.4lu (ASGN)\n", node->node_id);

      freenode (node->v.asgn.expr);
      node->v.asgn.expr = NULL;
      node->type = NODE_NOOP;
    }
}

optimize_fp pass2_opttab[] = {
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
  optimize_pass (2, node, pass2_opttab);
}


/* Pass 3: Substitution of constant variables */

#define VAR_IS_CONST(s) (s && s->v.var->entry_point && \
                         s->v.var->entry_point->v.expr->type == \
                         NODE_CONST)

static void
pass3_var (NODE *node)
{
  SYMBOL *s = node->v.symbol;

  if (VAR_IS_CONST (s))
    {
      if (verbose)
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

optimize_fp pass3_opttab[] = {
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
  optimize_pass (3, node, pass3_opttab);
}


/* Pass 4: Elimination of unused declarations */

static void
pass4a_var (NODE *node)
{
  node->v.symbol->ref_count++;
}

static void
pass4a_asgn (NODE *node)
{
  node->v.asgn.symbol->ref_count++;
}

optimize_fp pass4a_opttab[] = {
  NULL,        /* NODE_NOOP */
  NULL,        /* NODE_UNOP */
  NULL,        /* NODE_BINOP */
  NULL,        /* NODE_CONST */
  pass4a_var,  /* NODE_VAR */
  NULL,        /* NODE_CALL */
  pass4a_asgn, /* NODE_ASGN */
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
pass4b_vardecl (NODE *node)
{
  if (node->v.vardecl.symbol->ref_count == 0)
    {
      if (verbose)
	printf ("Removing unused %s variable %s (node %4.4lu)\n",
		node->v.vardecl.symbol->v.var->qualifier == QUA_GLOBAL ?
	        "global" : "automatic",
		node->v.vardecl.symbol->name,
		node->node_id);
      node->type = NODE_NOOP;
    }
}

optimize_fp pass4b_opttab[] = {
  NULL,           /* NODE_NOOP */
  NULL,           /* NODE_UNOP */
  NULL,           /* NODE_BINOP */
  NULL,           /* NODE_CONST */
  NULL,           /* NODE_VAR */
  NULL,           /* NODE_CALL */
  NULL,           /* NODE_ASGN */
  NULL,           /* NODE_EXPR */
  NULL,           /* NODE_RETURN */
  NULL,           /* NODE_PRINT */
  NULL,           /* NODE_JUMP */
  NULL,           /* NODE_COMPOUND  */
  NULL,           /* NODE_ITERATION */
  NULL,           /* NODE_CONDITION */
  pass4b_vardecl, /* NODE_VAR_DECL */
  NULL,           /* NODE_FNC_DECL */
};

void
optimize_pass_4 (NODE *node)
{
  optimize_pass (4, node, pass4a_opttab);
  optimize_pass (4, node, pass4b_opttab);
}


/* Pass 5: Elimination of ??? */

static void
pass5_condition (NODE *node)
{
  NODE *cond = node->v.condition.cond;

  if (cond->v.expr->type == NODE_CONST) {
    if (cond->v.expr->v.number == 1) /* TRUE */
      {
	if (verbose)
	  printf ("Eliminating conditional, node %4.4lu (always true)\n",
		  node->node_id);
	node->v.condition.iftrue_stmt->right = node->right;
	node->right = node->v.condition.iftrue_stmt;
	node->type = NODE_NOOP;
	// node->v.condition.iffalse_stmt = NULL;
	// freenode (node->v.condition.iffalse_stmt);
      }
    else if (cond->v.expr->v.number == 0) /* FALSE */
      {
	if (verbose)
	  printf ("Eliminating conditional, node %4.4lu (always false)\n",
		  node->node_id);
	node->v.condition.iffalse_stmt->right = node->right;
	node->right = node->v.condition.iffalse_stmt;
	node->type = NODE_NOOP;
      }
  }
}

optimize_fp pass5_opttab[] = {
  NULL,           /* NODE_NOOP */
  NULL,           /* NODE_UNOP */
  NULL,           /* NODE_BINOP */
  NULL,           /* NODE_CONST */
  NULL,           /* NODE_VAR */
  NULL,           /* NODE_CALL */
  NULL,           /* NODE_ASGN */
  NULL,           /* NODE_EXPR */
  NULL,           /* NODE_RETURN */
  NULL,           /* NODE_PRINT */
  NULL,           /* NODE_JUMP */
  NULL,           /* NODE_COMPOUND  */
  NULL,           /* NODE_ITERATION */
  pass5_condition,/* NODE_CONDITION */
  NULL,           /* NODE_VAR_DECL */
  NULL,           /* NODE_FNC_DECL */
};

static void
optimize_pass_5 (NODE *node)
{
  optimize_pass (5, node, pass5_opttab);
}


/* Mark & sweep: check *all* nodes */

optimize_fp mark_opttab[] = {
  mark_node,     /* NODE_NOOP */
  mark_node,     /* NODE_UNOP */
  mark_node,     /* NODE_BINOP */
  mark_node,     /* NODE_CONST */
  mark_node,     /* NODE_VAR */
  mark_node,     /* NODE_CALL */
  mark_node,     /* NODE_ASGN */
  mark_node,     /* NODE_EXPR */
  mark_node,     /* NODE_RETURN */
  mark_node,     /* NODE_PRINT */
  mark_node,     /* NODE_JUMP */
  mark_node,     /* NODE_COMPOUND  */
  mark_node,     /* NODE_ITERATION */
  mark_node,     /* NODE_CONDITION */
  mark_node,     /* NODE_VAR_DECL */
  mark_node      /* NODE_FNC_DECL */
};

static NODE *
mark_free (NODE *root)
{
  tmp_memory_pool = NULL;
  optimize (root, mark_opttab);
  return tmp_memory_pool;
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

  if (optimize_level > 1)
    {
      optimize_pass_4 (root);
      optimize_pass_5 (root);
    }
}

