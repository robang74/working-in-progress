/*******************************************************************************
 *
 * (C) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 *        THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************/

#ifndef SIGNED_OPCHECKS_H
#define SIGNED_OPCHECKS_H

#define TYPE_BITS(_t) (sizeof(_t) * CHAR_BIT)
#define SIGN_MASK(s_t, u_t) ((u_t)1 << (TYPE_BITS(s_t) - 1))
#define S_MAX_VAL(s_t, u_t) ((s_t)(SIGN_MASK(s_t, u_t) - 1))

#define S_UB_CSUM(a, b, st, ut) __extension__ ({     \
    st _a_ = (a); st _b_ = (b);                      \
    ut _s_ = (ut)_a_ + (ut)_b_;                      \
    ((_s_ ^ (ut)_a_) & (_s_ ^ (ut)_b_)               \
                     & SIGN_MASK(st, ut)) != 0;      \
}) /* overflow when return !0, positive logic */

#define S_UB_CDIFF(a, b, st, ut) __extension__ ({    \
    st _a_ = (a); st _b_ = (b);                      \
    ut _d_ = (ut)_a_ - (ut)_b_;                      \
    (((ut)_a_ ^ (ut)_b_) & ((ut)_a_ ^ _d_)           \
                         & SIGN_MASK(st, ut)) != 0;  \
})

#define S_UB_CMUL(a, b, st, ut) __extension__ ({     \
    st _a_ = (a); st _b_ = (b);                      \
    ut _ua_ = (_a_ < 0) ? -(ut)_a_ : (ut)_a_;        \
    ut _ub_ = (_b_ < 0) ? -(ut)_b_ : (ut)_b_;        \
    ut _lim_ = SIGN_MASK(st, ut);                    \
    int _neg_ = (_a_ < 0) != (_b_ < 0);              \
    ((_ub_ != 0) && (_ua_ > _lim_ / _ub_))           \
      || (!_neg_ && ((_ua_ == _lim_ && _ub_ == 1)    \
                  || (_ub_ == _lim_ && _ua_ == 1))); \
})

#endif
