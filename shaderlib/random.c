
/* Copyright (c) 1997 Lume Inc. */

#define _SGI_REENTRANT_FUNCTIONS
#include <stdlib.h>
#include <math.h>
#include <shader.h>
#include "random.h"

/* Random numbers are in range [-1..1] */


#ifdef _WIN32
int lume_random(int s)
{
  srand(s);

  return rand();
}
#else
int lume_random(int s)
{
  unsigned int t;

  t = s;
  rand_r((unsigned int *) &t);

  return t;
}
#endif



miScalar lume_random_1x1(miScalar f)
{
  int e;
  double m;
  int s;
  miScalar r;
  int t;

  m = frexp(f, &e);
  /* get mantissa and exponent to seed random num gen*/

  s = (int)(2000*m) + e;

  t = lume_random(s);
  r = ((abs(t) % 1000) / 500.0) - 1.0;

  return r;
}



miScalar lume_random_2x1(miScalar x, miScalar y)
{
  return lume_random_1x1(lume_random_1x1(x) +
			 lume_random_1x1(y));
}



miScalar lume_random_3x1(miVector p)
{
  return lume_random_1x1(lume_random_1x1(p.x) +
			 lume_random_1x1(p.y) +
			 lume_random_1x1(p.z));
}



miScalar lume_random_4x1(miVector p, miScalar t)
{
  return lume_random_1x1(lume_random_1x1(p.x) +
			 lume_random_1x1(p.y) +
			 lume_random_1x1(p.z) +
			 lume_random_1x1(t));
}


miScalar lume_random_5x1(miVector p, miScalar t, miScalar s)
{
  return lume_random_1x1(lume_random_1x1(p.x) +
			 lume_random_1x1(p.y) +
			 lume_random_1x1(p.z) +
			 lume_random_1x1(t) +
			 lume_random_1x1(s));
}



miVector lume_random_3x3(miVector p)
{
  miVector r;

  r.x = lume_random_4x1(p,1);
  r.y = lume_random_4x1(p,2);
  r.z = lume_random_4x1(p,3);

  return r;
}


miVector lume_random_4x3(miVector p, miScalar t)
{
  miVector r;

  r.x = lume_random_5x1(p,t,1);
  r.y = lume_random_5x1(p,t,2);
  r.z = lume_random_5x1(p,t,3);

  return r;
}
