
/* Copyright (c) 1997 Lume Inc. */

#include <shader.h>
#include <math.h>
#include "color.h"
#include "gamma.h"
#include "calc.h"

miColor red    = {0.7, 0.1, 0.1};
miColor orange = {0.7, 0.4, 0.1};
miColor yellow = {0.7, 0.7, 0.1};
miColor green  = {0.1, 0.7, 0.1};
miColor blue   = {0.1, 0.1, 0.7};
miColor violet = {0.4, 0.1, 0.7};


double intensity(miColor color)
{
  double i;
  
  i = autogamma_inverse_scalar(color.r) +
    autogamma_inverse_scalar(color.g) +
    autogamma_inverse_scalar(color.b);

  return autogamma_scalar(i);
}



void convert_rgb_hsv(miScalar r, miScalar g, miScalar b,
		     miScalar *h, miScalar *s, miScalar *v)
{
  miScalar min, max;
  
  max = max3(r,g,b);
  min = min3(r,g,b);

  *v = max;

  if (max != 0.0)
    *s = 1.0 - min / max;
  else
    *s = 0;

  if (*s == 0.0)
    *h = 0;   /* should be undefined.  sue me */

  else
    {
      miScalar d;

      d = max-min;
      
      if (r == max)
	*h = (g-b)/d;
      else if (g == max)
	*h = 2 + (b-r)/d;
      else if (b == max)
	*h = 4 + (r-g)/d;

      *h *= 60;

      if (*h < 0)
	*h += 360.0;
    }
}





void convert_hsv_rgb(miScalar h, miScalar s, miScalar v,
		     miScalar *r, miScalar *g, miScalar *b)
{
  if ( s == 0.0 )
    *r = *g = *b = v;
	
  else
    {
      int i;
      miScalar f;
      miScalar p;
      miScalar q;
      miScalar t;

      h = fmod( h, 360.0 );
      if ( h < 0.0 )
	h += 360.0;
      h /= 60.0;

      i = floor( h );
      f = h - i;

      p = v * ( 1.0 - s );
      q = v * ( 1.0 - ( s * f ) );
      t = v * ( 1.0 - ( s * ( 1.0 - f ) ) );
      
      switch ( i )
	{
	case 0:
	  *r = v; *g = t; *b = p;
	  break;
	case 1:
	  *r = q; *g = v; *b = p;
	  break;
	case 2:
	  *r = p; *g = v; *b = t;
	  break;
	case 3:
	  *r = p; *g = q; *b = v;
	  break;
	case 4:
	  *r = t; *g = p; *b = v;
	  break;
	case 5:
	  *r = v; *g = p; *b = q;
	  break;
	}

    }
}

