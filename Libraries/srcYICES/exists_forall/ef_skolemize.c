/*
 * This file is part of the Yices SMT Solver.
 * Copyright (C) 2020 SRI International.
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
 * Skolemization for the EF solver.
 */

#if defined(CYGWIN) || defined(MINGW)
#define EXPORTED __declspec(dllexport)
#define __YICES_DLLSPEC__ EXPORTED
#else
#define EXPORTED __attribute__((visibility("default")))
#endif

#include <stdint.h>
#include <stdio.h>

#include "exists_forall/ef_skolemize.h"

#include "yices.h"
#include "io/yices_pp.h"
#include "terms/term_explorer.h"


#define TRACE 0

/*
 * - t = skolemized term
 * - q = whether original term contained quantifiers or not
 */
typedef struct sk_pair_s {
  term_t t;
  bool q;
} sk_pair_t;

/*
 * Initialize the skolemize object
 */
void init_ef_skolemize(ef_skolemize_t *sk, ef_analyzer_t *analyzer, ef_prob_t *prob, bool f_ite, bool f_iff, bool ematching) {
  sk->analyzer = analyzer;
  sk->prob = prob;
  sk->flatten_ite = f_ite;
  sk->flatten_iff = f_iff;
  sk->ematching = ematching;

  sk->mgr = analyzer->manager;
  sk->terms = analyzer->terms;
  init_ivector(&sk->uvars, 10);
  init_ivector(&sk->aux, 10);
  sk->has_uvars = false;

  sk->failed = false;
  sk->unsupported = RESERVED_TERM; // anything will do.

  init_ptr_hmap(&sk->cache, 0);
}


/*
 * Delete the skolemize object
 */
void delete_ef_skolemize(ef_skolemize_t *sk) {
  sk->analyzer = NULL;
  sk->prob = NULL;
  sk->flatten_ite = false;
  sk->flatten_iff = false;
  sk->ematching = false;

  sk->mgr = NULL;
  sk->terms = NULL;
  delete_ivector(&sk->uvars);
  delete_ivector(&sk->aux);

  ptr_hmap_pair_t *p;
  ptr_hmap_t *map;

  map = &sk->cache;
  for (p = ptr_hmap_first_record(map);
       p != NULL;
       p = ptr_hmap_next_record(map, p)) {
    sk_pair_t* skp = p->val;
    safe_free(skp);
  }
  delete_ptr_hmap(map);
}


/*
 * Construct a composite term by updating its children.
 */
static term_t ef_update_composite(ef_skolemize_t *sk, term_t t, ivector_t *args) {
  term_manager_t *tm;
  term_table_t *terms;
  term_kind_t kind;
  term_t result;
  uint32_t n;
  term_t *c;

  assert(is_pos_term(t) && term_is_composite(sk->terms, t));

  tm = sk->mgr;
  terms = sk->terms;
  kind = term_kind(terms, t);
  n = args->size;
  c = args->data;

#if 0
  printf("Updating: %s\n", yices_term_to_string(t, 120, 1, 0));
  yices_pp_term_array(stdout, n, c, 120, 120, 0, 0);
#endif

  result = NULL_TERM;
  switch (kind) {
  case ARITH_EQ_ATOM:
  case EQ_TERM:            // equality
  case ARITH_BINEQ_ATOM:
    assert(n == 2);
    result = mk_eq(tm, c[0], c[1]);
    break;
  case ARITH_GE_ATOM:
    assert(n == 2);
    result = mk_arith_geq(tm, c[0], c[1]);
    break;
  case ARITH_IS_INT_ATOM:
    assert(n == 1);
    result = mk_arith_is_int(tm, c[0]);
    break;
  case ARITH_FLOOR:
    assert(n == 1);
    result = mk_arith_floor(tm, c[0]);
    break;
  case ARITH_CEIL:
    assert(n == 1);
    result = mk_arith_ceil(tm, c[0]);
    break;
  case ARITH_ABS:
    assert(n == 1);
    result = mk_arith_abs(tm, c[0]);
    break;

  case ITE_TERM:
  case ITE_SPECIAL:
    assert(n == 3);
    result = mk_ite(tm, c[0], c[1], c[2], term_type(tm->terms, t));
    break;

  case APP_TERM:
    result = mk_application(tm, c[0], n-1, c + 1);
    break;

  case TUPLE_TERM:
    result = mk_tuple(tm, n, c);
    break;
  case DISTINCT_TERM:
    result = mk_distinct(tm, n, c);
    break;

  case OR_TERM:            // n-ary OR
    assert(n > 1);
    result = mk_or(tm, n, c);
    break;
  case XOR_TERM:           // n-ary XOR
    result = mk_xor(tm, n, c);
    break;

  case ARITH_RDIV:
    assert(n == 2);
    result = mk_arith_rdiv(tm, c[0], c[1]);
    break;
  case ARITH_IDIV:
    assert(n == 2);
    result = mk_arith_idiv(tm, c[0], c[1]);
    break;
  case ARITH_MOD:
    assert(n == 2);
    result = mk_arith_mod(tm, c[0], c[1]);
    break;
  case ARITH_DIVIDES_ATOM:
    assert(n == 2);
    result = mk_arith_divides(tm, c[0], c[1]);
    break;

  case BV_ARRAY:
    assert(n >= 1);
    result = mk_bvarray(tm, n, c);
    break;
  case BV_DIV:
    assert(n == 2);
    result = mk_bvdiv(tm, c[0], c[1]);
    break;
  case BV_REM:
    assert(n == 2);
    result = mk_bvrem(tm, c[0], c[1]);
    break;
  case BV_SDIV:
    assert(n == 2);
    result = mk_bvsdiv(tm, c[0], c[1]);
    break;
  case BV_SREM:
    assert(n == 2);
    result = mk_bvsrem(tm, c[0], c[1]);
    break;
  case BV_SMOD:
    assert(n == 2);
    result = mk_bvsmod(tm, c[0], c[1]);
    break;
  case BV_SHL:
    assert(n == 2);
    result = mk_bvshl(tm, c[0], c[1]);
    break;
  case BV_LSHR:
    assert(n == 2);
    result = mk_bvlshr(tm, c[0], c[1]);
    break;
  case BV_ASHR:
    assert(n == 2);
    result = mk_bvashr(tm, c[0], c[1]);
    break;
  case BV_EQ_ATOM:
    assert(n == 2);
    result = mk_bveq(tm, c[0], c[1]);
    break;
  case BV_GE_ATOM:
    assert(n == 2);
    result = mk_bvge(tm, c[0], c[1]);
    break;
  case BV_SGE_ATOM:
    assert(n == 2);
    result = mk_bvsge(tm, c[0], c[1]);
    break;

  case ARITH_ROOT_ATOM:
  case LAMBDA_TERM:
  case UPDATE_TERM:
  case SELECT_TERM:
  case BIT_TERM:
  case POWER_PRODUCT:
  case ARITH_POLY:
  case BV64_POLY:
  case BV_POLY:
  default:
    //    printf("Unsupported term %s of kind %d\n", yices_term_to_string(t, 120, 120, 0), kind);
    //    assert(false);
    if (! sk->failed) {
      sk->failed = true;
      sk->unsupported = kind;
    }
    result = t; // we return t unchanged in this case.
    break;
  }

  return result;
}

/*
 * Create a name for a new skolem constant:
 * - the name is stored in ef's string buffer
 * - id is an integer that uniquely identifies the skolem constant
 * - origin is the original name of the existential variable being skolemized
 *
 * The skolem name is of the form "skolem<id>_<origin>".
 */
static void build_skolem_name(ef_analyzer_t *ef, uint32_t id, const char *origin) {
  string_buffer_reset(&ef->sbuffer);
  string_buffer_append_string(&ef->sbuffer, "skolem");
  string_buffer_append_uint32(&ef->sbuffer, id);
  if (origin != NULL) {
    string_buffer_append_char(&ef->sbuffer, '_');
    string_buffer_append_string(&ef->sbuffer, origin);
  }
  string_buffer_close(&ef->sbuffer);
}


/*
 * Pair to represent the result of skolemizing a term
 * - func is a skolem constant or function
 * - fapp is either the same as func (if that's a constant)
 *   of a term of the form (func ....).
 */
typedef struct ef_skolem_s {
  term_t func;
  term_t fapp;
} ef_skolem_t;


/*
 * Skolemize variable x using uvars as skolem arguments
 * - n = number of terms in uvars
 * - increment ef->num_skolem_funs if n>0.
 * - return a pair of terms (func, fapp):
 *    func = skolem function or skolem constant
 *    fapp = term that represents x
 *
 * - if n = 0:
 *    func is a skolem constant of the same type as x
 *    fapp is equal to func
 * - if n > 0
 *    func is a skolem function of type [s_1 ... s_n-> tau]
 *    fapp is the term (func uvars[0] ... unvars[n-1])
 *   where s_i = type of uvars[i-1] and tau = type of x.
 */
static ef_skolem_t ef_skolem_term(ef_analyzer_t *ef, term_t x, uint32_t n, term_t *uvars) {
  type_t *domt;
  type_t rt, tau;
  uint32_t i;
  term_table_t *terms;
  ef_skolem_t skolem;
  bool uint_skolem = true;

  terms = ef->terms;
  ef->num_skolem++;

  if (n == 0) {
    rt = term_type(terms, x);
    skolem.func = yices_new_uninterpreted_term(rt);
    skolem.fapp = skolem.func;
  }
  else {
    domt = (type_t *) safe_malloc(n * sizeof(type_t));
    for (i=0; i<n; i++) {
      tau = term_type(terms, uvars[i]);
      domt[i] = tau;
      uint_skolem = uint_skolem && (yices_type_is_bool(tau) || yices_type_is_scalar(tau) || yices_type_is_uninterpreted(tau));
    }
    rt = term_type(terms, x);

    type_t funct = yices_function_type(n, domt, rt);
    skolem.func = yices_new_uninterpreted_term(funct);
    skolem.fapp = yices_application(skolem.func, n, uvars);

    safe_free(domt); // BD: fix memory leak

    ef->num_skolem_funs ++;
  }
  build_skolem_name(ef, ef->num_skolem, yices_get_term_name(x));
  yices_set_term_name(skolem.func, ef->sbuffer.data);

  if (!uint_skolem) {
    ef->uint_skolem = false;
  }

#if TRACE
  printf("Skolemization: %s --> %s\n", yices_get_term_name(x), yices_term_to_string(skolem.fapp, 120, 1, 0));
#endif
  return skolem;
}


/*
 * Skolemize an existential term
 */
static term_t ef_skolem_body(ef_skolemize_t *sk, term_t t) {
  ef_analyzer_t *ef;
  term_table_t *terms;
  ivector_t *uvars;
  term_t body;
  composite_term_t *d;
  uint32_t i, n;
  term_t *a;

  ef = sk->analyzer;
  terms = sk->terms;
  uvars = &sk->uvars;

  /*
   * t is (NOT (FORALL x_0 ... x_k : body)
   * body is the last argument in the term descriptor
   */
  d = forall_term_desc(terms, t);
  n = d->arity - 1;
  assert(n >= 1);
  a = d->arg;
  body = opposite_term(d->arg[n]);

  term_subst_t subst;
  term_t *skolems;
  ef_skolem_t skolem;

  skolems = (term_t *) safe_malloc(n * sizeof(term_t));

  for (i = 0; i < n; i++) {
    assert(int_hmap_find(&ef->existentials, a[i]) == NULL);

    skolem = ef_skolem_term(ef, a[i], uvars->size, uvars->data);
    skolems[i] = skolem.fapp;
  }

  init_term_subst(&subst, ef->manager, n, a, skolems);
  body = apply_term_subst(&subst, body);
  delete_term_subst(&subst);

  safe_free(skolems);

  return body;
}

static inline term_t sk_update(sk_pair_t *sp, bool *is_quantified) {
  *is_quantified |= sp->q;
  return sp->t;
}

/*
 * Check whether we should apply distributivity to (or a[0] .... a[n-1)
 * - heuristic: return true if exactly one of a[i] is a conjunct
 */
static bool ef_distribute_is_cheap(ef_analyzer_t *ef, composite_term_t *d) {
  term_table_t *terms;
  uint32_t i, n;
  bool result;
  term_t t;

  terms = ef->terms;
  result = false;
  n = d->arity;
  for (i=0; i<n; i++) {
    t = d->arg[i];
    if (is_neg_term(t) && term_kind(terms, t) == OR_TERM) {
      // t is not (or ...) i.e., a conjunct
      result = !result;
      if (!result) break;  // second conjunct
    }
  }

  return result;
}

/*
 * Apply distributivity and flatten
 * - this function rewrites
 *     (or a[0] ... a[n-2] (and b[0] ... b[m-1]))
 *   to (and (or a[0] ... a[n-2] b[0])
 *            ...
 *           (or a[0] ... a[n-2] b[m-1]))
 *   then push all terms to aux and return
 *      (and (or a[0] ... a[n-1] b[j]) ...)
 *
 * - the rewriting is applied to the first a[j] that's a conjunct.
 */
static term_t ef_flatten_distribute(ef_analyzer_t *ef, composite_term_t *d, ivector_t *aux) {
  term_table_t *terms;
  composite_term_t *b;
  ivector_t *v;
  uint32_t i, j, k, n, m;
  term_t t;

  terms = ef->terms;

  j = 0; // Stop GCC warning

  /*
   * Find the first term among a[0 ... n-1] that's of the form (not (or ...))
   * - store that term's descriptor in b
   * - store its index in j
   */
  b = NULL;
  n = d->arity;
  for (i=0; i<n; i++) {
    t = d->arg[i];
    if (is_neg_term(t) && term_kind(terms, t) == OR_TERM && b == NULL) {
      b = or_term_desc(terms, t);
      j = i;
    }
  }

  /*
   * a[j] is (not (or b[0] ... b[m-1])) == not b
   * d->arg is (or a[0] ... a[n-1])
   */
  assert(b != NULL);

  ivector_reset(aux);

  v = &ef->aux;
  m = b->arity;
  for (k=0; k<m; k++) {
    /*
     * IMPORTANT: we make a full copy of d->arg into v
     * at every iteration of this loop. This is required because
     * mk_or modifies v->data.
     */
    ivector_reset(v);
    ivector_push(v, opposite_term(b->arg[k]));   // this is not b[k]
    for (i=0; i<n; i++) {
      if (i != j) {
        ivector_push(v, d->arg[i]); // a[i] for i/=j
      }
    }
    t = mk_or(ef->manager, v->size, v->data);  // t is (or b[i] a[0] ...)
    ivector_push(aux, t);
  }

  return mk_and(ef->manager, aux->size, aux->data);
}

/*
 * Convert a term to negated normal form and skolemize
 * - returns a pair <skolemized_t, is_quantified>
 *   is_quantified is used to decide flattening of Boolean conditions
 * - skolemize_t is the skolemization of t.
 * - is_quantified is true if t itself or one of its subterm is (forall ...) or (not (forall ...)).
 *
 * Side effects:
 * - the map t -> pair <skolemize_t, is_quant>  is stored in sk->cache
 * - sk->has_uvar is set to true if t is (forall ...)
 *   or if a subterm of t is a (forall ...) with positive polarity.
 *   or if some term of the from (Q ... (forall ...) ...) occurs in t.
 */
static sk_pair_t *ef_skolemize_term(ef_skolemize_t *sk, term_t t) {
  ptr_hmap_pair_t *r;

  r = ptr_hmap_get(&sk->cache, t);
  if (r->val != NULL) {
    return r->val;
  }

  sk_pair_t *sp_result, *sp;

  r->val = safe_malloc(sizeof(sk_pair_t));
  sp_result = r->val;

  term_manager_t *mgr;
  term_table_t *terms;
  term_kind_t kind;
  composite_term_t *d;
  uint32_t i, n;
  ivector_t args;
  term_t result, u, v;
  bool resultq = false;
  bool en_flattening;

  mgr = sk->mgr;
  terms = sk->terms;
  kind = term_kind(terms, t);
  result = NULL_TERM;
  en_flattening = true;

  if (!term_is_composite(terms, unsigned_term(t))) {
    result = t;
  } else {
    n = term_num_children(terms, t);
    init_ivector(&args, n);

    if (is_neg_term(t)) {
      switch (kind) {
      case ITE_TERM:
      case ITE_SPECIAL:
        d = ite_term_desc(terms, t);
        assert(d->arity == 3);
        if (en_flattening && is_boolean_term(terms, d->arg[1])) {
          assert(is_boolean_term(terms, d->arg[2]));
          /*
           * t is (not (ite C A B))
           *    u := (C and !A)
           *    v := (!C and !B)
           */
          u = mk_binary_and(mgr, d->arg[0], opposite_term(d->arg[1]));                // (C and !A)
          v = mk_binary_and(mgr, opposite_term(d->arg[0]), opposite_term(d->arg[2])); // (!C and !B)

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          sp = ef_skolemize_term(sk, v);
          v = sk_update(sp, &resultq);

          if (sk->flatten_ite || resultq) {
            result = mk_binary_or(mgr, u, v);
          } else {
            result = t;
          }
        }
        break;

      case EQ_TERM:
        d = eq_term_desc(terms, t);
        assert(d->arity == 2);
        if (en_flattening && is_boolean_term(terms, d->arg[0])) {
          assert(is_boolean_term(terms, d->arg[1]));
          /*
           * t is (not (iff A B))
           * flatten to (A and !B) or (!A and B)
           *
           */
          u = mk_binary_and(mgr, d->arg[0], opposite_term(d->arg[1])); // (A and !B)
          v = mk_binary_and(mgr, opposite_term(d->arg[0]), d->arg[1]); // (!A and B)

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          sp = ef_skolemize_term(sk, v);
          v = sk_update(sp, &resultq);

          if (sk->flatten_iff || resultq) {
            result = mk_binary_or(mgr, u, v);
          } else {
            result = t;
          }
        }
        break;

      case OR_TERM:
        d = or_term_desc(terms, t);
        /*
         * t is (not (or a[0] ... a[n-1]))
         * it flattens to (and (not a[0]) ... (not a[n-1]))
         */
        n = d->arity;
        for (i=0; i<n; i++) {
          u = opposite_term(d->arg[i]);

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          ivector_push(&args, u);
        }
        result = mk_and(mgr, n, args.data);
        break;

      case XOR_TERM:
        d = xor_term_desc(terms, t);
        /*
         * t is (not (xor a[0] ... a[n-1]))
         * it flattens to (not (xor (xor a[0] a[1]) a[2]) ... ) a[n-1]))
         */
        n = d->arity;
        u = NULL_TERM;
	assert(n > 0);
	u = d->arg[0];
	for (i=1; i<n; i++) {
	  u = mk_binary_xor(mgr, u, d->arg[i]);
        }
        v = opposite_term(u);
        sp = ef_skolemize_term(sk, v);
        v = sk_update(sp, &resultq);
        result = v;
        break;

      case FORALL_TERM:
        /*
         * t is (not (forall .. body))
         * it flattens to (exists .. (not body))
         */
        u = ef_skolem_body(sk, t);
        sp = ef_skolemize_term(sk, u);
        result = sk_update(sp, &resultq);
        resultq = true;
        break;

      default:
        break;
      }

      /*
       * BD: this handles non-boolean equality, distinct, and other composite
       * terms that return a Boolean.
       */
      if (result == NULL_TERM) {
	v = unsigned_term(t);
	n = term_num_children(terms, v);
        for (i=0; i<n; i++) {
          u = term_child(terms, v, i);
          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);
          ivector_push(&args, u);
        }
        result = opposite_term(ef_update_composite(sk, v, &args));
      }
    }
    else {
      switch (kind) {
      case ITE_TERM:
      case ITE_SPECIAL:
        d = ite_term_desc(terms, t);
        assert(d->arity == 3);
        if (en_flattening && is_boolean_term(terms, d->arg[1])) {
          assert(is_boolean_term(terms, d->arg[2]));
          /*
           * t is (ite C A B) = (u or v)
           *    u := (C and A)
           *    v := (not C and B)
           */
          u = mk_binary_and(mgr, d->arg[0], d->arg[1]);                // (C and A)
          v = mk_binary_and(mgr, opposite_term(d->arg[0]), d->arg[2]); // (not C) and B

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          sp = ef_skolemize_term(sk, v);
          v = sk_update(sp, &resultq);

          if (sk->flatten_ite || resultq) {
            result = mk_binary_or(mgr, u, v);
          }
          else {
            result = t;
          }
        }
        break;

      case EQ_TERM:
        d = eq_term_desc(terms, t);
        assert(d->arity == 2);
        if (en_flattening && is_boolean_term(terms, d->arg[0])) {
          assert(is_boolean_term(terms, d->arg[1]));
          /*
           * t is (iff A B)
           * flatten to (A and B) or (!A and !B)
           *
           */
          u = mk_binary_and(mgr, d->arg[0], d->arg[1]);                               // (u and v)
          v = mk_binary_and(mgr, opposite_term(d->arg[0]), opposite_term(d->arg[1])); // (!u and !v);

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          sp = ef_skolemize_term(sk, v);
          v = sk_update(sp, &resultq);

          if (sk->flatten_iff || resultq) {
            result = mk_binary_or(mgr, u, v);
          }
          else {
            result = t;
          }
        }
        break;

      case OR_TERM:
        n = term_num_children(terms, t);
        for(i=0; i<n; i++) {
          u = term_child(terms, t, i);

          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);

          ivector_push(&args, u);
        }
        result = ef_update_composite(sk, unsigned_term(t), &args);
        break;

      case XOR_TERM:
        d = xor_term_desc(terms, t);
        /*
         * t is (xor a[0] ... a[n-1])
         * it flattens to (xor (xor a[0] a[1]) a[2]) ... ) a[n-1])
         */
        n = d->arity;
        u = NULL_TERM;
	assert(n > 0);
	u = d->arg[0];
	for (i=1; i<n; i++) {
	  u = mk_binary_xor(mgr, u, d->arg[i]);
	}
        v = u;
        sp = ef_skolemize_term(sk, v);
        v = sk_update(sp, &resultq);
        result = v;
        break;

      case FORALL_TERM:
        /*
         * t is (forall .. body)
         * it flattens to body
         */
        sk->has_uvars = true;
        d = forall_term_desc(terms, t);
        n = d->arity - 1;
        for (i=0; i<n; i++) {
          ivector_push(&sk->uvars, d->arg[i]);
        }

        u = d->arg[n];
        sp = ef_skolemize_term(sk, u);
        result = sk_update(sp, &resultq);
        resultq = true;

        for (i=0; i<n; i++) {
          ivector_pop(&sk->uvars);
        }
        break;

      default:
        break;
      }

      /*
       * BD: this handles non-boolean equality, distinct, and other composite
       * terms that return a Boolean.
       */
      if (result == NULL_TERM) {
        n = term_num_children(terms, t);
        for (i=0; i<n; i++) {
          u = term_child(terms, t, i);
          sp = ef_skolemize_term(sk, u);
          u = sk_update(sp, &resultq);
          ivector_push(&args, u);
        }
        result = ef_update_composite(sk, unsigned_term(t), &args);
      }
    }

    delete_ivector(&args);
  }

  assert(result != NULL_TERM);

  if (is_pos_term(result) && term_kind(terms, result) == OR_TERM) {
    d = or_term_desc(terms, result);
    if (ef_distribute_is_cheap(sk->analyzer, d)) {
      result = ef_flatten_distribute(sk->analyzer, d, &sk->aux);
    }
  }

#if TRACE
  printf("Original (%d): %s\nSkolemized: %s\n", resultq,  yices_term_to_string(t, 120, 1, 0), yices_term_to_string(result, 120, 1, 0));
#endif

  sp_result->t = result;
  sp_result->q = resultq;
  return sp_result;
}


/*
 * Flattens nested (and .. (and .. ) .. ) and adds them to v
 */
static void ef_flatten_and(ef_skolemize_t *sk, term_t t, ivector_t *v) {
  composite_term_t *d;
  uint32_t i, n;
  term_t arg;

  if (is_neg_term(t) && term_kind(sk->terms, t) == OR_TERM) {
    // flatten top-level and into separate constraints

    /*
     * skolem is (not (or a[0] ... a[n-1]))
     * add (not a[0]), ..., (not a[n-1]) to vector v
     */

    d = or_term_desc(sk->terms, t);
    n = d->arity;

    for (i=0; i<n; i++) {
      arg = opposite_term(d->arg[i]);
      ef_flatten_and(sk, arg, v);
    }
  }
  else
    ivector_push(v, t);

}


/*
 * Get the skolemized version of term t:
 * - add the result to vector v
 * - this first computes the skolemization sk of term t
 * - if the term sk is a conjunction (i.e. of the form (NOT (OR ...)) then
 *   sk is flattened and the conjuncts are added to vector v
 * - if sk is not a conjunction, it's added to v as is.
 *
 * Side effects:
 * - sk->cache is updated to store mappings from t and its subterms to their skolemization
 * - sk->has_uvar is set to true if t contains (FORALL ...) subterms
 * - allocate a pattern descriptor for t in sk->prob->patterns if there isn't one already.
 *   (the pattern descriptor is a vector of terms).
 */
void ef_skolemize(ef_skolemize_t *sk, term_t t, ivector_t *v) {
  sk_pair_t *sp;
  term_t skolem;
  ptr_hmap_t *patterns;
  ptr_hmap_pair_t *p;

#if 0
  printf("Skolemizing: %s\n", yices_term_to_string(t, 120, 1, 0));
#endif

  sk->has_uvars = false;
  sp = ef_skolemize_term(sk, t);
  skolem = sp->t;

#if 0
  printf("Skolemized:  %s\n", yices_term_to_string(skolem, 120, 1, 0));
#endif

  if (sk->has_uvars) {
    // add empty pattern
    patterns = sk->prob->patterns;
    if (patterns != NULL) {
      p = ptr_hmap_get(patterns, t);
      if (p->val == NULL) {
        p->val = safe_malloc(sizeof(ivector_t));
        init_ivector(p->val, 1);
      }
    }
  }

  ef_flatten_and(sk, skolem, v);
}


/*
 * Skolemize patterns
 */
void ef_skolemize_patterns(ef_skolemize_t *sk) {
  ptr_hmap_t *patterns1;

  patterns1 = sk->prob->patterns;
  if (patterns1 != NULL && patterns1->nelems != 0) {
    ptr_hmap_t *patterns2;
    ptr_hmap_pair_t *r1, *r2;
    ivector_t *rv1, *rv2;
    uint32_t i, n;
    term_t *pdata;
    sk_pair_t *sp;
    term_t key2;

    patterns2 = (ptr_hmap_t *) safe_malloc(1 * sizeof(ptr_hmap_t));
    init_ptr_hmap(patterns2, 0);

    for (r1 = ptr_hmap_first_record(patterns1);
         r1 != NULL;
         r1 = ptr_hmap_next_record(patterns1, r1)) {
      rv1 = r1->val;
      n = rv1->size;

#if 0
      printf("Skolemizing pattern: \n");
      printf("  term: ");
      yices_pp_term(stdout, r1->key, 120, 1, 0);
      printf("  patterns (#%d): \n", n);
      yices_pp_term_array(stdout, n, rv1->data, 120, UINT32_MAX, 0, 0);
#endif

      sp = ef_skolemize_term(sk, r1->key);
      key2 = sp->t;

      r2 = ptr_hmap_get(patterns2, key2);
      if (r2->val == NULL) {
        r2->val = safe_malloc(sizeof(ivector_t));
        init_ivector(r2->val, rv1->size);
      }

      rv2 = r2->val;
      pdata = rv1->data;
      for (i=0; i<n; i++) {
        sp = ef_skolemize_term(sk, pdata[i]);
        ivector_push(rv2, sp->t);
      }

#if 0
      printf("Skolemized pattern: \n");
      printf("  term: ");
      yices_pp_term(stdout, r2->key, 120, 1, 0);
      printf("  patterns (#%d): \n", rv2->size);
      yices_pp_term_array(stdout, rv2->size, rv2->data, 120, UINT32_MAX, 0, 0);
#endif

    }

    delete_pattern_map(patterns1);
    safe_free(patterns1);
    sk->prob->patterns = patterns2;
  }
}


#if 0

// NOT USED

/*
 * Skolemize existentials in an analyzer
 */
term_t ef_analyzer_add_existentials(ef_analyzer_t *ef, bool toplevel, int_hmap_t *parent, term_t t) {
  uint32_t i, m, n;
  ivector_t uvars;
  int_hmap_pair_t *r;
  term_t p;
  composite_term_t *d;
  term_table_t *terms;
  term_t *a;
  term_t body;

  terms = ef->terms;
  init_ivector(&uvars, 10);

  /* the existential case
   * t is (NOT (FORALL x_0 ... x_k : body)
   * body is the last argument in the term descriptor
   */
  d = forall_term_desc(terms, t);
  n = d->arity - 1;
  assert(n >= 1);
  a = d->arg;
  body = opposite_term(d->arg[n]);

  if (!toplevel) {
    r = int_hmap_find(parent, t);
    while (r != NULL) {
      p = r->val;
      if (term_kind(terms, p) == FORALL_TERM) {
        if (is_pos_term(p)) {
          d = forall_term_desc(terms, p);
          m = d->arity;
          assert(m >= 2);
          for (i=0; i<m-1; i++) {
            ivector_push(&uvars, d->arg[i]);
          }
        }
      }
      r = int_hmap_find(parent, p);
    }
  }

  if (uvars.size == 0) {
    for (i=0; i<n; i++) {
      r = int_hmap_find(&ef->existentials, a[i]);
      assert(r == NULL);
      int_hmap_add(&ef->existentials, a[i], a[i]);
    }
  } else {
    term_subst_t subst;
    term_t *skolems;
    ef_skolem_t sk;

    skolems = (term_t *) safe_malloc(n * sizeof(term_t));

    for (i=0; i<n; i++) {
      r = int_hmap_find(&ef->existentials, a[i]);
      assert(r == NULL);

      sk = ef_skolem_term(ef, a[i], uvars.size, uvars.data);
      skolems[i] = sk.fapp;
      int_hmap_add(&ef->existentials, a[i], sk.func);
    }

    init_term_subst(&subst, ef->manager, n, a, skolems);
    body = apply_term_subst(&subst, body);
    delete_term_subst(&subst);

    safe_free(skolems);
  }

  delete_ivector(&uvars);
  return body;
}

#endif
