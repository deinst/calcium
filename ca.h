/*
    Copyright (C) 2020 Fredrik Johansson

    This file is part of Calcium.

    Calcium is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#ifndef CA_H
#define CA_H

#ifdef CA_INLINES_C
#define CA_INLINE
#else
#define CA_INLINE static __inline__
#endif

#include <stdio.h>
#include "flint/flint.h"
#include "antic/nf.h"
#include "antic/nf_elem.h"
#include "calcium.h"
#include "qqbar.h"
#include "fmpz_mpoly_q.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CA_INFO(ctx, args) do { if (ctx->options[CA_OPT_VERBOSE]) \
    do { flint_printf("%s:%d:  ", __FILE__, __LINE__); flint_printf args; } \
    while (0); } while (0);

/* Number object *************************************************************/

typedef union
{
    fmpq q;                           /* rational number */
    nf_elem_struct nf;                /* algebraic number field element */
    fmpz_mpoly_q_struct * mpoly_q;    /* generic field element */
}
ca_elem_struct;

typedef struct
{
    ulong field;
    ca_elem_struct elem;
}
ca_struct;

typedef ca_struct ca_t[1];
typedef ca_struct * ca_ptr;
typedef const ca_struct * ca_srcptr;

#define CA_FMPQ(x)         (&((x)->elem.q))
#define CA_MPOLY_Q(x)      (&(((x)->elem.mpoly_q)[0]))
#define CA_NF_ELEM(x)      (&((x)->elem.nf))
#define CA_FMPQ_NUMREF(x)  (fmpq_numref(CA_FMPQ(x)))
#define CA_FMPQ_DENREF(x)  (fmpq_denref(CA_FMPQ(x)))

/* We always allocate QQ and QQ(i), with field index 0 and 1 */
#define CA_FIELD_ID_QQ       0
#define CA_FIELD_ID_QQ_I     1

/* Bits added to the top of field_id to encode nonnumbers */
#define CA_UNKNOWN        (UWORD(1) << (FLINT_BITS - 1))
#define CA_UNDEFINED      (UWORD(1) << (FLINT_BITS - 2))
#define CA_UNSIGNED_INF   (UWORD(1) << (FLINT_BITS - 3))
#define CA_SIGNED_INF     (UWORD(1) << (FLINT_BITS - 4))
#define CA_SPECIAL        (CA_UNKNOWN | CA_UNDEFINED | CA_UNSIGNED_INF | CA_SIGNED_INF)

#define CA_IS_SPECIAL(x)  ((x)->field & CA_SPECIAL)

/* Field object **************************************************************/

typedef enum
{
    /* The rational field QQ.
       Field elements are represented as fmpq_t */
    CA_FIELD_TYPE_QQ,

    /* Algebraic number field QQ(a), a = qqbar_t.
       Field elements are represented as nf_elem_t. */
    CA_FIELD_TYPE_NF,

    /* Transcendental(?) number field QQ(x) with generating element
       x = func(c1,...,cn) where func is a symbolic function, c_i are ca_t.
       Field elements are represented as fmpz_mpoly_q_t (could be fmpz_poly_q_t ...). */
    CA_FIELD_TYPE_FUNC,

    /* Generic multivariate field QQ(x1,...,xn), x1,...,xn defined by
       reference to other fields of univariate type;
       field elements are represented as fmpz_mpoly_q_t. */
    CA_FIELD_TYPE_MULTI
}
ca_field_type_t;

typedef struct
{
    qqbar_struct x;     /* qqbar_t element */
    nf_struct nf;       /* antic number field for fast arithmetic */
}
ca_field_description_nf;

typedef struct
{
    ulong func;             /* f = F_Pi, F_Exp, ... */
    ca_struct * args;       /* Function arguments x1, ..., xn. */
    slong args_len;         /* Number of function arguments n. */
    acb_struct enclosure;   /* Numerical enclosure of f(x1,...,xn) */
}
ca_field_description_func;

typedef struct
{
    slong len;                  /* Number of generators */
    slong * ext;                /* Indices to generators in the context object */
    fmpz_mpoly_struct * ideal;  /* Algebraic relations for reduction */
    slong ideal_len;            /* Number of relations for reduction */
}
ca_field_description_multi;

typedef union
{
    ca_field_description_nf nf;
    ca_field_description_func func;
    ca_field_description_multi multi;
}
ca_field_description_struct;

typedef struct
{
    ca_field_type_t type;
    ca_field_description_struct data;
}
ca_field_struct;

typedef ca_field_struct ca_field_t[1];

#define CA_FIELD_NF(K) (&((K)->data.nf.nf))
#define CA_FIELD_NF_QQBAR(K) (&((K)->data.nf.x))

#define CA_FIELD_MCTX(K, ctx) (&((ctx)->mctx[(K)->data.multi.len - 1]))

/* Context object ************************************************************/

/* Could also be configurable. */
#define CA_MPOLY_ORD ORD_LEX

enum
{
    CA_OPT_VERBOSE,
    CA_OPT_PREC_LIMIT,
    CA_OPT_QQBAR_DEG_LIMIT,
    CA_OPT_NUM_OPTIONS
};

typedef struct
{
    ca_field_struct * fields;     /* Cached extension fields */
    slong fields_len;
    slong fields_alloc;

    fmpz_mpoly_ctx_struct * mctx;  /* Cached contexts for multivariate polys */
    slong mctx_len;

    slong * options;
}
ca_ctx_struct;

typedef ca_ctx_struct ca_ctx_t[1];

/* Context management */

void ca_ctx_init(ca_ctx_t ctx);
void ca_ctx_clear(ca_ctx_t ctx);
void ca_ctx_print(const ca_ctx_t ctx);

/* Field methods */

/* todo: all take ctx; update and check docs */
void ca_field_init_qq(ca_field_t K);
void ca_field_init_nf(ca_field_t K, const qqbar_t x);
void ca_field_init_const(ca_field_t K, ulong func);
void ca_field_init_fx(ca_field_t K, ulong func, const ca_t x, ca_ctx_t ctx);
void ca_field_init_multi(ca_field_t K, slong len, ca_ctx_t ctx);
void ca_field_clear(ca_field_t K, ca_ctx_t ctx);

void ca_field_set_ext(ca_field_t K, slong i, slong x_index, ca_ctx_t ctx);
void ca_field_print(const ca_field_t K, const ca_ctx_t ctx);
int ca_field_cmp(const ca_field_t K1, const ca_field_t K2, ca_ctx_t ctx);

slong _ca_ctx_get_field_const(ca_ctx_t ctx, ulong func);
slong _ca_ctx_get_field_fx(ca_ctx_t ctx, ulong func, const ca_t x);

/* Numbers */

void ca_init(ca_t x, ca_ctx_t ctx);
void ca_clear(ca_t x, ca_ctx_t ctx);
void ca_swap(ca_t x, ca_t y, ca_ctx_t ctx);
void _ca_make_field_element(ca_t x, slong i, ca_ctx_t ctx);

ca_ptr ca_vec_init(slong n, ca_ctx_t ctx);
void ca_vec_clear(ca_ptr v, slong n, ca_ctx_t ctx);


CA_INLINE void
_ca_make_fmpq(ca_t x, ca_ctx_t ctx)
{
    if (x->field != CA_FIELD_ID_QQ)
        _ca_make_field_element(x, CA_FIELD_ID_QQ, ctx);
}

void ca_set(ca_t res, const ca_t x, ca_ctx_t ctx);

void ca_zero(ca_t x, ca_ctx_t ctx);
void ca_one(ca_t x, ca_ctx_t ctx);

void ca_set_si(ca_t x, slong v, ca_ctx_t ctx);
void ca_set_ui(ca_t x, ulong v, ca_ctx_t ctx);
void ca_set_fmpz(ca_t x, const fmpz_t v, ca_ctx_t ctx);
void ca_set_fmpq(ca_t x, const fmpq_t v, ca_ctx_t ctx);

void ca_i(ca_t x, ca_ctx_t ctx);
void ca_pi(ca_t res, ca_ctx_t ctx);
void ca_pi_i(ca_t res, ca_ctx_t ctx);

void ca_unknown(ca_t x, ca_ctx_t ctx);

void ca_undefined(ca_t x, ca_ctx_t ctx);
void ca_uinf(ca_t x, ca_ctx_t ctx);
void ca_pos_inf(ca_t x, ca_ctx_t ctx);
void ca_neg_inf(ca_t x, ca_ctx_t ctx);
void ca_pos_i_inf(ca_t x, ca_ctx_t ctx);
void ca_neg_i_inf(ca_t x, ca_ctx_t ctx);

void ca_set_qqbar(ca_t res, const qqbar_t x, ca_ctx_t ctx);

void ca_print(const ca_t x, const ca_ctx_t ctx);

/* Random generation */

void ca_randtest_rational(ca_t res, flint_rand_t state, slong bits, ca_ctx_t ctx);
void ca_randtest(ca_t res, flint_rand_t state, slong depth, slong bits, ca_ctx_t ctx);
void ca_randtest_special(ca_t res, flint_rand_t state, slong depth, slong bits, ca_ctx_t ctx);

/* Representation properties */

CA_INLINE int ca_is_unknown(const ca_t x, ca_ctx_t ctx)
{
    return x->field == CA_UNKNOWN;
}

/* Value predicates and comparisons */

truth_t ca_check_is_number(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_zero(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_one(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_neg_one(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_i(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_neg_i(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_algebraic(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_rational(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_integer(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_real(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_imaginary(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_nonreal(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_undefined(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_infinity(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_uinf(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_signed_inf(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_pos_inf(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_neg_inf(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_pos_i_inf(const ca_t x, ca_ctx_t ctx);
truth_t ca_check_is_neg_i_inf(const ca_t x, ca_ctx_t ctx);

truth_t ca_check_equal(const ca_t x, const ca_t y, ca_ctx_t ctx);
truth_t ca_check_lt(const ca_t x, const ca_t y, ca_ctx_t ctx);
truth_t ca_check_le(const ca_t x, const ca_t y, ca_ctx_t ctx);
truth_t ca_check_gt(const ca_t x, const ca_t y, ca_ctx_t ctx);
truth_t ca_check_ge(const ca_t x, const ca_t y, ca_ctx_t ctx);

int ca_equal_repr(const ca_t x, const ca_t y, ca_ctx_t ctx);

/* Field structure operations */

void ca_merge_fields(ca_t resx, ca_t resy, const ca_t x, const ca_t y, ca_ctx_t ctx);
void ca_condense_field(ca_t res, ca_ctx_t ctx);

/* Arithmetic */

void ca_neg(ca_t res, const ca_t x, ca_ctx_t ctx);

void ca_add_fmpq(ca_t res, const ca_t x, const fmpq_t y, ca_ctx_t ctx);
void ca_add_fmpz(ca_t res, const ca_t x, const fmpz_t y, ca_ctx_t ctx);
void ca_add_ui(ca_t res, const ca_t x, ulong y, ca_ctx_t ctx);
void ca_add_si(ca_t res, const ca_t x, slong y, ca_ctx_t ctx);
void ca_add(ca_t res, const ca_t x, const ca_t y, ca_ctx_t ctx);

void ca_sub_fmpq(ca_t res, const ca_t x, const fmpq_t y, ca_ctx_t ctx);
void ca_sub_fmpz(ca_t res, const ca_t x, const fmpz_t y, ca_ctx_t ctx);
void ca_sub_ui(ca_t res, const ca_t x, ulong y, ca_ctx_t ctx);
void ca_sub_si(ca_t res, const ca_t x, slong y, ca_ctx_t ctx);
void ca_sub(ca_t res, const ca_t x, const ca_t y, ca_ctx_t ctx);

void ca_mul_fmpq(ca_t res, const ca_t x, const fmpq_t y, ca_ctx_t ctx);
void ca_mul_fmpz(ca_t res, const ca_t x, const fmpz_t y, ca_ctx_t ctx);
void ca_mul_ui(ca_t res, const ca_t x, ulong y, ca_ctx_t ctx);
void ca_mul_si(ca_t res, const ca_t x, slong y, ca_ctx_t ctx);
void ca_mul(ca_t res, const ca_t x, const ca_t y, ca_ctx_t ctx);

void ca_inv(ca_t res, const ca_t x, ca_ctx_t ctx);

void ca_fmpq_div(ca_t res, const fmpq_t x, const ca_t y, ca_ctx_t ctx);
void ca_fmpz_div(ca_t res, const fmpz_t x, const ca_t y, ca_ctx_t ctx);
void ca_ui_div(ca_t res, ulong x, const ca_t y, ca_ctx_t ctx);
void ca_si_div(ca_t res, slong x, const ca_t y, ca_ctx_t ctx);

void ca_div_fmpq(ca_t res, const ca_t x, const fmpq_t y, ca_ctx_t ctx);
void ca_div_fmpz(ca_t res, const ca_t x, const fmpz_t y, ca_ctx_t ctx);
void ca_div_ui(ca_t res, const ca_t x, ulong y, ca_ctx_t ctx);
void ca_div_si(ca_t res, const ca_t x, slong y, ca_ctx_t ctx);
void ca_div(ca_t res, const ca_t x, const ca_t y, ca_ctx_t ctx);

/* Elementary functions */

void ca_exp(ca_t res, const ca_t x, ca_ctx_t ctx);
void ca_log(ca_t res, const ca_t x, ca_ctx_t ctx);

/* Numerical evaluation */

void ca_get_acb_raw(acb_t res, const ca_t x, slong prec, ca_ctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif

