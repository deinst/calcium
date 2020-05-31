/*
    Copyright (C) 2020 Fredrik Johansson

    This file is part of Calcium.

    Calcium is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "ca.h"

truth_t
ca_check_is_pos_inf(const ca_t x, ca_ctx_t ctx)
{
    if (CA_IS_SPECIAL(x))
    {
        if (ca_is_unknown(x, ctx))
            return T_UNKNOWN;

        if (x->field & CA_SIGNED_INF)
        {
            ca_t t;
            t->field = x->field & ~CA_SIGNED_INF;
            t->elem = x->elem;
            return ca_check_is_one(t, ctx);
        }

        return T_FALSE;
    }

    return T_FALSE;
}