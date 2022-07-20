/*
 * This file is part of the Yices SMT Solver.
 * Copyright (C) 2017 SRI International.
 *
 * Yices is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Yices is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Yices.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * PATTERN COMPILER FOR E-MATCHING
 */

#if defined(CYGWIN) || defined(MINGW)
#define EXPORTED __declspec(dllexport)
#define __YICES_DLLSPEC__ EXPORTED
#else
#define EXPORTED __attribute__((visibility("default")))
#endif

#include "solvers/quant/ematch_compile.h"
#include "yices.h"

#define TRACE 0

#if TRACE

#include <stdio.h>

#include "solvers/cdcl/smt_core_printer.h"
#include "solvers/egraph/egraph_printer.h"

#include "io/yices_pp.h"
#include "terms/term_explorer.h"

#endif


/*
 * Initialize pattern compiler
 */
void init_ematch_compiler(ematch_compile_t *comp, ematch_instr_table_t *itbl, term_table_t *terms) {
  init_int_hmap(&comp->W[0], 0);
  init_int_hmap(&comp->W[1], 0);
  init_int_hmap(&comp->W[2], 0);
  init_int_hmap(&comp->W[3], 0);

  init_int_hmap(&comp->V, 0);

  init_int_queue(&comp->patterns, 0);

  comp->o = 0;
  comp->itbl = itbl;
  comp->terms = terms;
}

/*
 * Reset pattern compiler
 */
void reset_ematch_compiler(ematch_compile_t *comp) {
  int_hmap_reset(&comp->W[0]);
  int_hmap_reset(&comp->W[1]);
  int_hmap_reset(&comp->W[2]);
  int_hmap_reset(&comp->W[3]);

  int_hmap_reset(&comp->V);

  int_queue_reset(&comp->patterns);

  comp->o = 0;
  comp->itbl = NULL;
  comp->terms = NULL;
}

/*
 * Delete pattern compiler
 */
void delete_ematch_compiler(ematch_compile_t *comp) {
  delete_int_hmap(&comp->W[0]);
  delete_int_hmap(&comp->W[1]);
  delete_int_hmap(&comp->W[2]);
  delete_int_hmap(&comp->W[3]);

  delete_int_hmap(&comp->V);

  delete_int_queue(&comp->patterns);

  comp->o = 0;
  comp->itbl = NULL;
  comp->terms = NULL;
}


/***********************
 *   PATTERN COMPILER  *
 **********************/

#if TRACE
static void ematch_print_W(ematch_compile_t *comp, const char *comment) {
  int_hmap_t *W;
  int_hmap_pair_t *ip;

  W = &comp->W[0];
  if (W->nelems > 0) {
    printf("COMP.W (compare) #%d %s\n", W->nelems, comment);
    for (ip = int_hmap_first_record(W);
         ip != NULL;
         ip = int_hmap_next_record(W, ip)) {
      if (ip->key >= 0) {
        printf("  %d -> ", ip->key);
        yices_pp_term(stdout, ip->val, 120, 1, 0);
      }
    }
  }

  W = &comp->W[1];
  if (W->nelems > 0) {
    printf("COMP.W (check) #%d %s\n", W->nelems, comment);
    for (ip = int_hmap_first_record(W);
         ip != NULL;
         ip = int_hmap_next_record(W, ip)) {
      if (ip->key >= 0) {
        printf("  %d -> ", ip->key);
        yices_pp_term(stdout, ip->val, 120, 1, 0);
      }
    }
  }

  W = &comp->W[2];
  if (W->nelems > 0) {
    printf("COMP.W (filter) #%d %s\n", W->nelems, comment);
    for (ip = int_hmap_first_record(W);
         ip != NULL;
         ip = int_hmap_next_record(W, ip)) {
      if (ip->key >= 0) {
        printf("  %d -> ", ip->key);
        yices_pp_term(stdout, ip->val, 120, 1, 0);
      }
    }
  }

  W = &comp->W[3];
  if (W->nelems > 0) {
    printf("COMP.W (other) #%d %s\n", W->nelems, comment);
    for (ip = int_hmap_first_record(W);
         ip != NULL;
         ip = int_hmap_next_record(W, ip)) {
      if (ip->key >= 0) {
        printf("  %d -> ", ip->key);
        yices_pp_term(stdout, ip->val, 120, 1, 0);
      }
    }
  }

}
#endif

void ematch_print_instr(FILE *f, ematch_instr_table_t *itbl, int32_t idx, bool recursive) {
  ematch_instr_t *instr;

  assert(idx >= 0 && idx < itbl->ninstr);
  instr = &itbl->data[idx];
//  recursive = false;

  switch(instr->op) {
  case EMATCH_INIT:
    fprintf(f, "    instr%d: init(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  case EMATCH_BIND:
    fprintf(f, "    instr%d: bind(%d, %s, %d, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  case EMATCH_CHECK:
    fprintf(f, "    instr%d: check(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  case EMATCH_COMPARE:
    fprintf(f, "    instr%d: compare(%d, %d, instr%d)\n", idx, instr->i, instr->j, instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  case EMATCH_YIELD: {
      int32_t i, n;
      n = instr->nsubs;
      fprintf(f, "    instr%d: yield(#%d entries: ", idx, n);
      for (i=0; i<n; i++) {
        fprintf(f, "%s -> %d, ", yices_term_to_string(instr->vdata[i], 120, 1, 0), instr->idata[i]);
      }
      fprintf(f, ")\n");
    }
    break;
  case EMATCH_FILTER:
    fprintf(f, "    instr%d: filter(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  case EMATCH_CHOOSEAPP:
    fprintf(f, "    instr%d: choose-app(%d, instr%d, %d)\n", idx, instr->o, instr->next, instr->j);
    break;
  case EMATCH_CONTINUE:
    fprintf(f, "    instr%d: continue(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
    if (recursive)
      ematch_print_instr(f, itbl, instr->next, recursive);
    break;
  default:
//    fprintf(f, "Unsupported ematch instruction instr%d of type: %d\n", idx, instr->op);
    assert(0);
  }
}

/*
 * Compile constant
 */
static int32_t ematch_compile_const(ematch_compile_t *comp, int32_t i, term_t t) {
  ematch_instr_table_t *itbl;
  int32_t idx, next;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data + idx;

  assert(term_kind(comp->terms, t) != VARIABLE);
  assert(term_kind(comp->terms, t) != APP_TERM);

  instr->op = EMATCH_CHECK;
  instr->i = i;
  instr->f = t;

#if 0
  printf("    (pre) instr%d: check(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
#endif

  next = ematch_compile(comp);
  instr = itbl->data + idx;
  instr->next = next;

#if 0
  printf("    instr%d: check(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
#endif

  assert(instr->idx == idx);
  assert(idx >= 0);

  return idx;
}

/*
 * Compile variable
 */
static int32_t ematch_compile_var(ematch_compile_t *comp, int32_t i, term_t x) {
  int32_t idx;

  int_hmap_t *V;
  int_hmap_pair_t *ip;

  V = &comp->V;
  ip = int_hmap_find(V, x);
  if (ip == NULL) {
    int_hmap_add(V, x, i);

    idx = ematch_compile(comp);

    // Undo changes to comp
    ip = int_hmap_find(V, x);
    int_hmap_erase(V, ip);

  } else {
    ematch_instr_table_t *itbl;
    ematch_instr_t *instr;
    int32_t next;

    itbl = comp->itbl;
    idx = ematch_instr_table_alloc(itbl);
    instr = itbl->data + idx;

    instr->op = EMATCH_COMPARE;
    instr->i = i;
    instr->j = ip->val;

#if 0
    printf("    (pre) instr%d: compare(%d, %d, instr%d)\n", idx, instr->i, instr->j, instr->next);
#endif

    next = ematch_compile(comp);
    instr = itbl->data + idx;
    instr->next = next;

#if 0
    printf("    instr%d: compare(%d, %d, instr%d)\n", idx, instr->i, instr->j, instr->next);
#endif

    assert(instr->idx == idx);
  }

  assert(idx >= 0);

  return idx;
}

/*
 * Compile filter
 */
static int32_t ematch_compile_filter(ematch_compile_t *comp, int32_t i, term_t f) {
  ematch_instr_table_t *itbl;
  int32_t idx, next;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data + idx;

  instr->op = EMATCH_FILTER;
  instr->i = i;
  instr->f = f;

#if 0
  printf("    (pre) instr%d: filter(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
#endif

  next = ematch_compile(comp);
  instr = itbl->data + idx;
  instr->next = next;

#if 0
  printf("    instr%d: filter(%d, %s, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->next);
#endif

  assert(instr->idx == idx);
  assert(idx >= 0);

  return idx;
}

static void ematch_add_to_W(ematch_compile_t *comp, int32_t i, term_t t) {
  term_table_t *terms;

  terms = comp->terms;
  switch (term_kind(terms, t)) {
    case VARIABLE:
      int_hmap_add(&comp->W[0], i, t);
      break;

    case CONSTANT_TERM:
    case ARITH_CONSTANT:
    case BV64_CONSTANT:
    case BV_CONSTANT:
      int_hmap_add(&comp->W[1], i, t);
      break;

    case UNINTERPRETED_TERM:
      if (is_function_term(terms, t)) {
//        printf("Unexpected term (kind %d): ", term_kind(terms, t));
//        yices_pp_term(stdout, t, 120, 1, 0);
        assert(false);
      } else {
        int_hmap_add(&comp->W[1], i, t);
      }
      break;

    case APP_TERM:
      int_hmap_add(&comp->W[2], i, composite_term_arg(terms, t, 0));
      // fall-through intended
    default:
      int_hmap_add(&comp->W[3], i, t);
    }
}

/*
 * Compile function application
 */
static int32_t ematch_compile_fapp(ematch_compile_t *comp, int32_t i, term_t f) {
  ematch_instr_table_t *itbl;
  int32_t idx, next, j;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data+ idx;

  assert(term_kind(comp->terms, f) == APP_TERM);
  instr->op = EMATCH_BIND;

  composite_term_t *app;
  uint32_t n, offset;

  app = app_term_desc(comp->terms, f);
  n = app->arity - 1;
  offset = comp->o;

  for(j=0; j<n; j++) {
    ematch_add_to_W(comp, offset+j, app->arg[j+1]);
  }
  comp->o = offset + n;

  instr->i = i;
  instr->f = app->arg[0];
  instr->o = offset;

#if 0
  printf("    (pre) instr%d: bind(%d, %s, %d, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
#endif

  next = ematch_compile(comp);
  instr = itbl->data + idx;
  instr->next = next;

#if 0
  printf("    instr%d: bind(%d, %s, %d, instr%d)\n", idx, instr->i, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
#endif

  // Undo changes to comp
//  comp->o = offset;

  assert(instr->idx == idx);
  assert(idx >= 0);

  return idx;
}

/*
 * Compile continue
 */
static int32_t ematch_compile_continue(ematch_compile_t *comp, term_t f) {
  ematch_instr_table_t *itbl;
  int32_t idx, next, j;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data+ idx;

  assert(term_kind(comp->terms, f) == APP_TERM);
  instr->op = EMATCH_CONTINUE;

  composite_term_t *app;
  uint32_t n, offset;

  app = app_term_desc(comp->terms, f);
  n = app->arity - 1;
  offset = comp->o;

  for(j=0; j<n; j++) {
    ematch_add_to_W(comp, offset+j, app->arg[j+1]);
  }
  comp->o = offset + n;

  instr->f = app->arg[0];
  instr->o = offset;

#if 0
  printf("    (pre) instr%d: continue(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
#endif

  next = ematch_compile(comp);
  instr = itbl->data + idx;
  instr->next = next;

#if 0
  printf("    instr%d: continue(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
#endif

  // Undo changes to comp
//  comp->o = offset;

  assert(instr->idx == idx);
  assert(idx >= 0);

  return idx;
}

/*
 * Compile empty set
 */
static int32_t ematch_compile_empty(ematch_compile_t *comp) {
  ematch_instr_table_t *itbl;
  int32_t idx;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data + idx;

  instr->op = EMATCH_YIELD;

  int_hmap_t *V;
  uint32_t i, n;
  int_hmap_pair_t *ip;

  V = &comp->V;
  n = V->nelems;
  instr->vdata = (term_t *) safe_malloc(n * sizeof(term_t));
  instr->idata = (int32_t *) safe_malloc(n * sizeof(int32_t));
  instr->nsubs = n;
  i = 0;
  for (ip = int_hmap_first_record(V);
       ip != NULL;
       ip = int_hmap_next_record(V, ip)) {
    if (ip->key >= 0) {
      instr->vdata[i] = ip->key;
      instr->idata[i] = ip->val;
      i++;
    }
  }

#if 0
  printf("    instr%d: yield(#%d entries: ", idx, instr->nsubs);
  for (i=0; i<n; i++) {
    printf("%d <- %s, ", instr->idata[i], yices_term_to_string(instr->vdata[i], 120, 1, 0));
  }
  printf(")\n");
#endif

  assert(instr->idx == idx);
  assert(idx >= 0);

  return idx;
}

/*
 * Compile based on working set
 */
int32_t ematch_compile(ematch_compile_t *comp) {
  int32_t idx;
  int_hmap_pair_t *ip;
  int32_t i, j;
  term_t x;
  int_hmap_t *W;

  idx = -1;
  i = -1;
  x = NULL_TERM;
  for(j=0; j<4; j++) {
    W = &comp->W[j];
    for (ip = int_hmap_first_record(W);
         ip != NULL;
         ip = int_hmap_next_record(W, ip)) {
      if (ip->key > 0) {
        i = ip->key;
        x = ip->val;
        int_hmap_erase(W, ip);
        break;
      }
    }
    if (i != -1)
      break;
  }

  if (i == -1) {
    if (int_queue_is_empty(&comp->patterns)) {
      idx = ematch_compile_empty(comp);
    } else {
      x = int_queue_pop(&comp->patterns);
      idx = ematch_compile_continue(comp, x);
    }
  } else {
    term_table_t *terms;
    terms = comp->terms;

#if 0
    printf("  choosing %d (kind: %d) -> ", i, term_kind(terms, x));
    yices_pp_term(stdout, x, 120, 1, 0);
#endif

    switch(term_kind(terms, x)) {
    case CONSTANT_TERM:
    case ARITH_CONSTANT:
    case BV64_CONSTANT:
    case BV_CONSTANT:
      idx = ematch_compile_const(comp, i, x);
      break;

    case UNINTERPRETED_TERM:
      if (is_function_term(terms, x)) {
        idx = ematch_compile_filter(comp, i, x);
      } else {
        idx = ematch_compile_const(comp, i, x);
      }
      break;

    case VARIABLE:
      idx = ematch_compile_var(comp, i, x);
      break;

    case APP_TERM:
      idx = ematch_compile_fapp(comp, i, x);
      break;

    case ARITH_EQ_ATOM:
    case EQ_TERM:            // equality
    case ARITH_BINEQ_ATOM:
    case BV_EQ_ATOM:
    case ITE_TERM:
    case ITE_SPECIAL:
    case DISTINCT_TERM:
    case OR_TERM:            // n-ary OR
    case XOR_TERM:           // n-ary XOR
//      printf("Unexpected term (kind %d): ", term_kind(terms, x));
//      yices_pp_term(stdout, x, 120, 1, 0);
      assert(false);
      break;

    default:
//      printf("Unsupported term (kind %d): ", term_kind(terms, x));
//      yices_pp_term(stdout, x, 120, 1, 0);
      assert(false);
    }

  }

  assert(idx >= 0);

  return idx;
}

/*
 * Compile function
 */
static int32_t ematch_compile_func(ematch_compile_t *comp, composite_term_t *app) {
  ematch_instr_table_t *itbl;
  int32_t idx, next;
  ematch_instr_t *instr;

  itbl = comp->itbl;
  idx = ematch_instr_table_alloc(itbl);
  instr = itbl->data + idx;

  instr->op = EMATCH_INIT;

  uint32_t j, n, offset;

  n = app->arity;
  offset = comp->o;

  instr->f = app->arg[0];
  instr->o = offset;

  for(j=1; j<n; j++) {
    ematch_add_to_W(comp, offset+j, app->arg[j]);
  }
  comp->o = offset + n;

#if 0
  printf("    (pre) instr%d: init(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
//  ematch_print_W(comp, "(func: post)");
#endif

  next = ematch_compile(comp);
  instr = itbl->data + idx;
  instr->next = next;

#if 0
  printf("    instr%d: init(%s, %d, instr%d)\n", idx, yices_term_to_string(instr->f, 120, 1, 0), instr->o, instr->next);
#endif

  // Undo changes to comp
//  comp->o = offset;

  assert(instr->idx == idx);
  assert(itbl->data[idx].next == instr->next);
  assert(idx >= 0);

  return idx;
}

/*
 * Compile multi pattern
 */
static int32_t ematch_compile_multi(ematch_compile_t *comp, uint32_t n, term_t *a) {
  uint32_t i;
  term_t pat;

  assert(n > 1);

  for (i=1; i<n; i++) {
    int_queue_push(&comp->patterns, a[i]);
  }

  pat = a[0];

  return ematch_compile_func(comp, app_term_desc(comp->terms, pat));
}

/*
 * Compile pattern to an instruction sequence
 * - returns an index in the instruction table
 */
int32_t ematch_compile_pattern(ematch_compile_t *comp, term_t pat) {
  int32_t idx;
  term_table_t *terms;
  term_kind_t kind;
  composite_term_t *d;

  assert(comp->V.nelems == 0);

  idx = -1;
  terms = comp->terms;
  kind = term_kind(terms, pat);
  if (kind == APP_TERM) {
#if TRACE
    printf("  pattern: ");
    yices_pp_term(stdout, pat, 120, 1, 0);
    printf("    offset: %d\n", comp->o);
#endif

    idx = ematch_compile_func(comp, app_term_desc(terms, pat));

#if TRACE
    printf("    code: instr%d\n", idx);
    ematch_print_instr(stdout, comp->itbl, idx, true);
//    printf("    offset (new): %d\n", comp->o);
#endif
  } else if (kind == TUPLE_TERM) {
#if TRACE
    printf("  pattern (multi): ");
    yices_pp_term(stdout, pat, 120, 1, 0);
    printf("    offset: %d\n", comp->o);
#endif

    d = tuple_term_desc(terms, pat);
    idx = ematch_compile_multi(comp, d->arity, d->arg);

#if TRACE
    printf("    code: instr%d\n", idx);
    ematch_print_instr(stdout, comp->itbl, idx, true);
//    printf("    offset (new): %d\n", comp->o);
#endif
  } else {
//    printf("Unsupported pattern term (kind %d): ", kind);
//    yices_pp_term(stdout, pat, 120, 1, 0);
    assert(false);
  }

  return idx;
}

