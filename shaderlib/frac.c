
/* Copyright (c) 1997 Lume Inc. */

#include <math.h>
#include <shader.h>
#include "frac.h"


struct frac *frac_create(double lacunarity, double h, int octaves)
{
  struct frac *frac;
  int i;
  double total;

  frac = mi_mem_allocate(sizeof (struct frac));

  frac->coefficient = mi_mem_allocate(sizeof (double) * octaves);
  frac->frequency   = mi_mem_allocate(sizeof (double) * octaves);
  frac->octaves     = octaves;

  for (i = 0; i < octaves; i++)
    {
      double f;

      f = pow(lacunarity, i);

      (*frac->frequency)[i]   = f;
      (*frac->coefficient)[i] = pow(f, -h);
    }

  /* Scale, so that the energy of the sum is 1 */

  total = 0;

  for (i = 0; i < octaves; i++)
    total += (*frac->coefficient)[i] * (*frac->coefficient)[i];

  for (i = 0; i < octaves; i++)
    (*frac->coefficient)[i] /= total;

  return frac;
}  



void frac_destroy(struct frac *frac)
{
  mi_mem_release(frac->coefficient);
  mi_mem_release(frac->frequency);
  mi_mem_release(frac);
}



miScalar frac_scalar(struct frac *frac, miVector p)
{
  int i;
  double t;

  t = 0;

  for (i=0; i < frac->octaves; i++)
    {
      miVector fp;

      fp = p;
      mi_vector_mul(&fp, (*frac->frequency)[i]);

      t += (*frac->coefficient)[i] * (2.0 * mi_noise_3d(&fp) - 1.0);
    }

  return t;
}


miScalar frac_scalar_positive(struct frac *frac, miVector p)
{
  int i;
  double t;

  t = 0;

  for (i=0; i < frac->octaves; i++)
    {
      miVector fp;

      fp = p;
      mi_vector_mul(&fp, (*frac->frequency)[i]);

      t += (*frac->coefficient)[i] * mi_noise_3d(&fp);
    }

  return t;
}
