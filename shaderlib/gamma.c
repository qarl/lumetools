
/* Copyright (c) 1997 Lume Inc. */

#include <shader.h>
#include <math.h>
#include <stdlib.h>
#include "gamma.h"

static double gamma_correction = DEFAULT_GAMMA;
static int gamma_init = 0;

#define GAMMA() (gamma_init ? gamma_correction : init_gamma_correction())


double init_gamma_correction()
{
  double gamma;
  char *gamma_str;

  gamma_str = getenv(GAMMA_VARIABLE);

  if (gamma_str)
    gamma = atof(gamma_str);
  else
    gamma = DEFAULT_GAMMA;

  /* define global variable */
  gamma_correction = gamma;
  gamma_init = 1;

  return gamma;
}


miScalar autogamma_scalar(miScalar x)
{
  if ((x==0.0) || (x==1.0))
    return x;

  return pow(x,GAMMA());
}


miScalar autogamma_inverse_scalar(miScalar x)
{
  if ((x==0.0) || (x==1.0))
    return x;

  return pow(x,1/GAMMA());
}

miColor autogamma_color(miColor color)
{
  miColor gamma_color;

  gamma_color.r = autogamma_scalar(color.r);
  gamma_color.g = autogamma_scalar(color.g);
  gamma_color.b = autogamma_scalar(color.b);
  
  return gamma_color;
}

