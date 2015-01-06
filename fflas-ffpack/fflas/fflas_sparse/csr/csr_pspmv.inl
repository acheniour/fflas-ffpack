/* -*- mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
/*
 * Copyright (C) 2014 the FFLAS-FFPACK group
 *
 * Written by   Bastien Vialla <bastien.vialla@lirmm.fr>
 *
 *
 * ========LICENCE========
 * This file is part of the library FFLAS-FFPACK.
 *
 * FFLAS-FFPACK is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 * ========LICENCE========
 *.
 */

#ifndef __FFLASFFPACK_fflas_sparse_CSR_pspmv_INL
#define __FFLASFFPACK_fflas_sparse_CSR_pspmv_INL

#ifdef __FFLASFFPACK_USE_TBB
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#endif

namespace FFLAS {
namespace sparse_details_impl {
template <class Field>
inline void pfspmv(const Field &F, const Sparse<Field, SparseMatrix_t::CSR> &A,
                   typename Field::ConstElement_ptr x,
                   typename Field::Element_ptr y, FieldCategories::GenericTag) {
    int step = __FFLASFFPACK_CACHE_LINE_SIZE / sizeof(typename Field::Element);
#if defined(__FFLASFFPACK_USE_TBB)
    tbb::parallel_for(tbb::blocked_range<index_t>(0, A.m, step),
                      [&F, &A, &x, &y](const tbb::blocked_range<index_t> &r) {
        for (index_t i = r.begin(), end = r.end(); i < end; ++i) {
            auto start = A.st[i], stop = A.st[i + 1];
            index_t j = 0;
            index_t diff = stop - start;
            typename Field::Element y1, y2, y3, y4;
            F.assign(y1, F.zero);
            F.assign(y2, F.zero);
            F.assign(y3, F.zero);
            F.assign(y4, F.zero);
            for (; j < ROUND_DOWN(diff, 4); j += 4) {
                F.axpyin(y1, A.dat[start + j], x[A.col[start + j]]);
                F.axpyin(y2, A.dat[start + j + 1], x[A.col[start + j + 1]]);
                F.axpyin(y3, A.dat[start + j + 2], x[A.col[start + j + 2]]);
                F.axpyin(y4, A.dat[start + j + 3], x[A.col[start + j + 3]]);
            }
            for (; j < diff; ++j) {
                F.axpyin(y1, A.dat[start + j], x[A.col[start + j]]);
            }
            F.addin(y[i], y1);
            F.addin(y[i], y2);
            F.addin(y[i], y3);
            F.addin(y[i], y4);
        }
    });
#else
// The minimum size has to be a multiple of cache_line/sizeof(Element) to avoid
// cache coherency problem (ex: 8 for double, 16 for float)
#pragma omp parallel for
    for (index_t i = 0; i < A.m; ++i) {
        auto start = A.st[i], stop = A.st[i + 1];
        index_t j = 0;
        index_t diff = stop - start;
        typename Field::Element y1, y2, y3, y4;
        F.assign(y1, F.zero);
        F.assign(y2, F.zero);
        F.assign(y3, F.zero);
        F.assign(y4, F.zero);
        for (; j < ROUND_DOWN(diff, 4); j += 4) {
            F.axpyin(y1, A.dat[start + j], x[A.col[start + j]]);
            F.axpyin(y2, A.dat[start + j + 1], x[A.col[start + j + 1]]);
            F.axpyin(y3, A.dat[start + j + 2], x[A.col[start + j + 2]]);
            F.axpyin(y4, A.dat[start + j + 3], x[A.col[start + j + 3]]);
        }
        for (; j < diff; ++j) {
            F.axpyin(y1, A.dat[start + j], x[A.col[start + j]]);
        }
        F.addin(y[i], y1);
        F.addin(y[i], y2);
        F.addin(y[i], y3);
        F.addin(y[i], y4);
    }
#endif
}

template <class Field>
inline void pfspmv(const Field &F, const Sparse<Field, SparseMatrix_t::CSR> &A,
                   typename Field::ConstElement_ptr x,
                   typename Field::Element_ptr y,
                   FieldCategories::UnparametricTag) {
    int step = __FFLASFFPACK_CACHE_LINE_SIZE / sizeof(typename Field::Element);
#if defined(__FFLASFFPACK_USE_TBB)
    tbb::parallel_for(tbb::blocked_range<index_t>(0, A.m, step),
                      [&F, &A, &x, &y](const tbb::blocked_range<index_t> &r) {
        for (index_t i = r.begin(), end = r.end(); i < end; ++i) {
            auto start = A.st[i], stop = A.st[i + 1];
            index_t j = 0;
            index_t diff = stop - start;
            typename Field::Element y1 = 0, y2 = 0, y3 = 0, y4 = 0;
            for (; j < ROUND_DOWN(diff, 4); j += 4) {
                y1 += A.dat[start + j] * x[A.col[start + j]];
                y2 += A.dat[start + j + 1] * x[A.col[start + j + 1]];
                y3 += A.dat[start + j + 2] * x[A.col[start + j + 2]];
                y4 += A.dat[start + j + 3] * x[A.col[start + j + 3]];
            }
            for (; j < diff; ++j) {
                y1 += A.dat[start + j] * x[A.col[start + j]];
            }
            y[i] += y1 + y2 + y3 + y4;
        }
    });
#else
#pragma omp parallel for schedule(static, 8)
    for (index_t i = 0; i < A.m; ++i) {
        auto start = A.st[i], stop = A.st[i + 1];
        index_t j = 0;
        index_t diff = stop - start;
        typename Field::Element y1 = 0, y2 = 0, y3 = 0, y4 = 0;
        for (; j < ROUND_DOWN(diff, 4); j += 4) {
            y1 += A.dat[start + j] * x[A.col[start + j]];
            y2 += A.dat[start + j + 1] * x[A.col[start + j + 1]];
            y3 += A.dat[start + j + 2] * x[A.col[start + j + 2]];
            y4 += A.dat[start + j + 3] * x[A.col[start + j + 3]];
        }
        for (; j < diff; ++j) {
            y1 += A.dat[start + j] * x[A.col[start + j]];
        }
        y[i] += y1 + y2 + y3 + y4;
    }
#endif
}

template <class Field>
inline void pfspmv(const Field &F, const Sparse<Field, SparseMatrix_t::CSR> &A,
                   typename Field::ConstElement_ptr x,
                   typename Field::Element_ptr y, const int64_t kmax) {
    int step = __FFLASFFPACK_CACHE_LINE_SIZE / sizeof(typename Field::Element);
#if defined(__FFLASFFPACK_USE_TBB)
    tbb::parallel_for(
        tbb::blocked_range<index_t>(0, A.m, step),
        [&F, &A, &x, &y, &kmax](const tbb::blocked_range<index_t> &r) {
            for (index_t i = r.begin(), end = r.end(); i < end; ++i) {
                index_t j = A.st[i];
                index_t j_loc = j;
                index_t j_end = A.st[i + 1];
                index_t block = (j_end - j_loc) / kmax;
                for (index_t l = 0; l < (index_t)block; ++l) {
                    j_loc += kmax;
                    for (; j < j_loc; ++j) {
                        y[i] += A.dat[j] * x[A.col[j]];
                    }
                    F.reduce(y[i]);
                }
                for (; j < j_end; ++j) {
                    y[i] += A.dat[j] * x[A.col[j]];
                }
                F.reduce(y[i]);
            }
        });
#else
#pragma omp parallel for
    for (index_t i = 0; i < A.m; ++i) {
        index_t j = A.st[i];
        index_t j_loc = j;
        index_t j_end = A.st[i + 1];
        index_t block = (j_end - j_loc) / kmax;
        for (index_t l = 0; l < (index_t)block; ++l) {
            j_loc += kmax;
            for (; j < j_loc; ++j) {
                y[i] += A.dat[j] * x[A.col[j]];
            }
            F.reduce(y[i]);
        }
        for (; j < j_end; ++j) {
            y[i] += A.dat[j] * x[A.col[j]];
        }
        F.reduce(y[i]);
    }
#endif
}

template <class Field, class Func>
inline void
pfspmv(const Field &F, const Sparse<Field, SparseMatrix_t::CSR_ZO> &A,
       typename Field::ConstElement_ptr x, typename Field::Element_ptr y,
       Func &&func, FieldCategories::GenericTag) {
    int step = __FFLASFFPACK_CACHE_LINE_SIZE / sizeof(typename Field::Element);
#if defined(__FFLASFFPACK_USE_TBB)
    tbb::parallel_for(
        tbb::blocked_range<index_t>(0, A.m, step),
        [&F, &A, &x, &y, &func](const tbb::blocked_range<index_t> &r) {
            for (index_t i = r.begin(), end = r.end(); i < end; ++i) {
                auto start = A.st[i], stop = A.st[i + 1];
                index_t j = 0;
                index_t diff = stop - start;
                typename Field::Element y1, y2, y3, y4;
                F.assign(y1, F.zero);
                F.assign(y2, F.zero);
                F.assign(y3, F.zero);
                F.assign(y4, F.zero);
                for (; j < ROUND_DOWN(diff, 4); j += 4) {
                    func(y1, x[A.col[start + j]]);
                    func(y2, x[A.col[start + j + 1]]);
                    func(y3, x[A.col[start + j + 2]]);
                    func(y4, x[A.col[start + j + 3]]);
                }
                for (; j < diff; ++j) {
                    func(y1, x[A.col[start + j]]);
                }
                F.addin(y[i], y1);
                F.addin(y[i], y2);
                F.addin(y[i], y3);
                F.addin(y[i], y4);
            }
        });
#else
#pragma omp parallel for
    for (index_t i = 0; i < A.m; ++i) {
        auto start = A.st[i], stop = A.st[i + 1];
        index_t j = 0;
        index_t diff = stop - start;
        typename Field::Element y1, y2, y3, y4;
        F.assign(y1, F.zero);
        F.assign(y2, F.zero);
        F.assign(y3, F.zero);
        F.assign(y4, F.zero);
        for (; j < ROUND_DOWN(diff, 4); j += 4) {
            func(y1, x[A.col[start + j]]);
            func(y2, x[A.col[start + j + 1]]);
            func(y3, x[A.col[start + j + 2]]);
            func(y4, x[A.col[start + j + 3]]);
        }
        for (; j < diff; ++j) {
            func(y1, x[A.col[start + j]]);
        }
        F.addin(y[i], y1);
        F.addin(y[i], y2);
        F.addin(y[i], y3);
        F.addin(y[i], y4);
    }

#endif
}

template <class Field, class Func>
inline void
pfspmv(const Field &F, const Sparse<Field, SparseMatrix_t::CSR_ZO> &A,
       typename Field::ConstElement_ptr x, typename Field::Element_ptr y,
       Func &&func, FieldCategories::UnparametricTag) {
    int step = __FFLASFFPACK_CACHE_LINE_SIZE / sizeof(typename Field::Element);
#if defined(__FFLASFFPACK_USE_TBB)
    tbb::parallel_for(
        tbb::blocked_range<index_t>(0, A.m, step),
        [&F, &A, &x, &y, func](const tbb::blocked_range<index_t> &r) {
            for (index_t i = r.begin(), end = r.end(); i < end; ++i) {
                auto start = A.st[i], stop = A.st[i + 1];
                index_t j = 0;
                index_t diff = stop - start;
                typename Field::Element y1 = 0, y2 = 0, y3 = 0, y4 = 0;
                for (; j < ROUND_DOWN(diff, 4); j += 4) {
                    func(y1, x[A.col[start + j]]);
                    func(y2, x[A.col[start + j + 1]]);
                    func(y3, x[A.col[start + j + 2]]);
                    func(y4, x[A.col[start + j + 3]]);
                }
                for (; j < diff; ++j) {
                    func(y1, x[A.col[start + j]]);
                }
                y[i] += y1 + y2 + y3 + y4;
            }
        });
#else
#pragma omp parallel for
    for (index_t i = 0; i < A.m; ++i) {
        auto start = A.st[i], stop = A.st[i + 1];
        index_t j = 0;
        index_t diff = stop - start;
        typename Field::Element y1 = 0, y2 = 0, y3 = 0, y4 = 0;
        for (; j < ROUND_DOWN(diff, 4); j += 4) {
            func(y1, x[A.col[start + j]]);
            func(y2, x[A.col[start + j + 1]]);
            func(y3, x[A.col[start + j + 2]]);
            func(y4, x[A.col[start + j + 3]]);
        }
        for (; j < diff; ++j) {
            func(y1, x[A.col[start + j]]);
        }
        y[i] += y1 + y2 + y3 + y4;
    }
#endif
}
} // CSR_details

} // FFLAS

#endif //  __FFLASFFPACK_fflas_CSR_pspmv_INL