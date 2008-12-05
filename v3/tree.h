/*
   V3: tree.h

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

#ifndef _TREE_H
#define _TREE_H

#include "symbol.h"

enum node_type
{
  NODE_NOOP,      /* Noop, do nothing */
  NODE_UNOP,      /* Unary operation */
  NODE_BINOP,     /* Binary operation */
  NODE_CONST,     /* Constant */
  NODE_VAR,       /* Variable reference */
  NODE_CALL,      /* Function call */
  NODE_ASGN,      /* Assignment */
  NODE_EXPR,      /* Expression */
  NODE_RETURN,    /* The return statement */
  NODE_PRINT,     /* The print statement */
  NODE_JUMP,      /* The jump statements */
  NODE_COMPOUND , /* The compound statement */
  NODE_ITERATION, /* Iteration statement */
  NODE_CONDITION, /* Condition statement */
  NODE_VAR_DECL,  /* Variable declaration */
  NODE_FNC_DECL   /* Function declaration */
};

enum opcode_type
{
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_DIV,
  OPCODE_NEG,
  OPCODE_AND,
  OPCODE_NOT,
  OPCODE_OR,
  OPCODE_EQ,
  OPCODE_NE,
  OPCODE_LT,
  OPCODE_GT,
  OPCODE_LE,
  OPCODE_GE
};

enum jump_type
{
  JUMP_BREAK,
  JUMP_CONTINUE,
};

struct node_struct
{
  struct node_struct *memory_link;
  struct node_struct *left;
  struct node_struct *right;

  unsigned long node_id;          /* Used while printing the parse tree */
  enum node_type type;

  union {
    enum opcode_type opcode;      /* type == NODE_UNOP
                                     || type == NODE_BINOP */

    long number;                  /* type == NODE_CONST */
    SYMBOL *symbol;               /* type == NODE_VAR */
    struct node_struct *expr;     /* type == NODE_EXPR
                                     || type == NODE_COMPOUND
                                     || type == NODE_RETURN
                                     || type == NODE_PRINT */

    struct {
      SYMBOL *symbol;
      struct arglist_struct *args;
    } funcall;                    /* type == NODE_CALL */

    struct {
      SYMBOL *symbol;
      struct node_struct *expr;
    } asgn;                       /* type == NODE_ASGN */

    struct {
      enum jump_type type;
      unsigned level;
    } jump;                       /* type == NODE_JUMP */

    struct {
      struct node_struct *cond;
      struct node_struct *stmt;
    } iteration;                  /* type == NODE_ITERATION */

    struct {
      struct node_struct *cond;
      struct node_struct *iftrue_stmt;
      struct node_struct *iffalse_stmt;
    } condition;                  /* type == NODE_CONDITION */

    struct {
      SYMBOL *symbol;
      struct node_struct *expr;
    } vardecl;                    /* type == NODE_VAR_DECL */

    struct {
      SYMBOL *symbol;
      struct node_struct *stmt;
    } fncdecl;                    /* type == NODE_FNC_DECL */

  } v;
};

struct arglist_struct
{
  struct arglist_struct *next;
  struct node_struct *node;
};

typedef struct node_struct NODE;
typedef struct arglist_struct ARGLIST;

/* Generalized traversal interface */
typedef void (*traverse_fp)(NODE *);

/* Global variables */
extern NODE *root;  /* the root of a parse tree */
extern unsigned int nodes_counter;

/* Function prototypes */
NODE *addnode (enum node_type);
void freenode (NODE *);
void free_all_nodes (void);
ARGLIST *make_arglist (NODE *, ARGLIST *);

void traverse (NODE *, traverse_fp *);

unsigned int get_last_node_id (void);
void print_tree (NODE *);
void print_node (NODE *);

#endif /* not _TREE_H */

