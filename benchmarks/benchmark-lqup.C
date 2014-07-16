/* -*- mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s


/* Copyright (c) FFLAS-FFPACK
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* ========LICENCE========
*/

#include <iostream>

#include "fflas-ffpack/fflas-ffpack.h"
#include "fflas-ffpack/field/modular-balanced.h"
#include "fflas-ffpack/utils/timer.h"
#include "fflas-ffpack/utils/Matio.h"


using namespace std;

int main(int argc, char** argv) {

  // parameter: p, n, iteration, file

  int    p    = atoi(argv[1]);
  int n    = atoi(argv[2]);
  size_t iter = atoi(argv[3]);


  typedef FFPACK::Modular<double> Field;
  typedef Field::Element Element;

  Field F(p);

  Timer chrono;
  double time=0.0;

  Element *A;

  for (size_t i=0;i<iter;++i){

    if (argc > 4){
      A = read_field (F, argv[4], &n, &n);
    }
    else{
      A = new Element[n*n];
      Field::RandIter G(F);
      for (size_t j=0; j< (size_t)n*n; ++j)
	G.random(*(A+j));
    }

    size_t * P = new size_t[n];
    size_t * Q = new size_t[n];

    chrono.clear();
    chrono.start();
    FFPACK::LUdivine (F, FFLAS::FflasNonUnit, FFLAS::FflasNoTrans, n, n, A, n,
		      P, Q);
    chrono.stop();

    time+=chrono.usertime();
    delete[] P;
    delete[] Q;
    delete[] A;

  }

  cerr<<"n: "<<n<<" p: "<<p<<" time: "<<time/(double)iter<<endl;


  return 0;
}
