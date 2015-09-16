
/* Copyright (c) 1997 Lume Inc. */

#include <math.h>
#include "calc.h"


double clamp(double x, double min, double max)
{

  if (x < min)
    return min;

  if (x > max)
    return max;

  return x;
}


double range_scale(double x,
		   double orig_min, double orig_max,
		   double target_min, double target_max)
{
  if (x < orig_min)
    return target_min;
 
  if (x > orig_max)
    return target_max;

  if (x == orig_min)
    return target_min;

  return target_min + 
    (x - orig_min) / (orig_min - orig_max) * (target_min - target_max);

}





/* bias - shifts the middle region to b:
   bias(0.0,b) = 0, bias(0.5,b) = b, bias(1.0,b) = 1
   bias(x,0.5) is I */

double bias(double x, double b)
{
  return pow(x, log(b)/-0.69314718);
}




/* gain - pushes a function out to its edges, or pulls it back in.
   (contrast).
   gain(x,0.5) is I */

double gain(double x,double g)
{
  if (x < 0.5)
    return bias(2.0 * x, 1.0 - g) / 2.0;
  else
    return 1.0 - bias(2.0 - 2.0 * x, 1.0 - g) / 2.0;
}




/* soft_clamp - is nearly I at 0, but has a horizontal asymptote at
   y=max */

double soft_clamp(double x,double max)
{
  if (x >= 0)
    return max * (1.0 - exp(-x/max));
  else
    return -max * (1.0 - exp(x/max));
    
}

