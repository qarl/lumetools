
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <gamma.h>
#include <lic.h>

#define NAME "Illumination"
#define PROD PROD_LUMELIGHT


struct Illumination_param
{
  struct soft_spot si_default;

  miScalar brightness;

  miBoolean falloff_none_on;
  miBoolean falloff_real_on;

  miBoolean falloff_linear_on;
  miScalar  falloff_linear_start;
  miScalar  falloff_linear_end;

  miBoolean falloff_custom_on;
  miScalar  falloff_custom_start;
  miScalar  falloff_custom_mid;
  miScalar  falloff_custom_end;

};



DLLEXPORT void Illumination_init(miState *state, 
			  struct Illumination_param *paras,
			  miBoolean *req)
{
  if (req)
    {
      /* global shader init here, paras == NULL */

      *req = miTRUE;

      lic(state, NAME, PROD);
    }

  else
    {
      /* instance init here, paras valid */

    }
}




DLLEXPORT void Illumination_exit(struct Illumination_param *paras)
{
  if (!paras)
    {
      /* global cleanup */
      lic_end(PROD);
    }

  else
    {
      /* instance cleanup */
    }

}




static double custom(double x, struct Illumination_param *paras)
{
  double s,m,p,b;

  if (x <= paras->falloff_custom_start)
    return 1;
  if (x >= paras->falloff_custom_end)
    return 0;

  s = paras->falloff_custom_end - paras->falloff_custom_start;

  m = (paras->falloff_custom_mid - paras->falloff_custom_start)/s;
  p = (x - paras->falloff_custom_start)/s;


  b = 1.0 - bias(p,m);
  
  return b;

}





DLLEXPORT miBoolean Illumination(miColor *result, miState *state,
		    struct Illumination_param *paras)
{
  struct soft_spot copy;
  miScalar power;
  miBoolean light;
  int type_infinite, type_spot, type_point;

  type_infinite = type_spot = type_point = 0;

  /* determine the type */
  if (state->dist == 0.0)
    type_infinite = 1;
  else
    {
      if ((paras->si_default.direction.x) ||
	  (paras->si_default.direction.y) ||
	  (paras->si_default.direction.z) )

	type_spot = 1;
      else
	type_point = 1;
    }


  if (type_infinite)
    { /* Can't have falloff with infinite lights */
      paras->falloff_none_on = miTRUE;
      paras->falloff_real_on = paras->falloff_linear_on = 
	paras->falloff_custom_on = miFALSE;
    }


  copy = paras->si_default;
  copy.atten = miFALSE;   /* turn off SoftImage's falloff */

	    

  /* "Switch" on falloff type, and determine power multiplier */

  if (paras->falloff_none_on)
    power = paras->brightness;


  if (paras->falloff_linear_on)
    power = paras->brightness * range_scale(state->dist,
					    paras->falloff_linear_start, 
					    paras->falloff_linear_end,
					    1,0);
					   
  if (paras->falloff_real_on)
    power = autogamma_scalar(9.0 * paras->brightness * paras->brightness /
			     state->dist / state->dist);


  if (paras->falloff_custom_on)
    power = custom(state->dist, paras);


  /* Has the light become negligible? */
  if (power < 0.001)
    return miFALSE;


  /* Now do our do for the lights */
  if (type_spot)
    light = soft_spot(result, state, &copy);

  if (type_point)
    light = soft_point(result, state, 
		       (struct soft_point *)&copy);

  if (type_infinite)
    light = soft_infinite(result, state, 
			  (struct soft_infinite *)&copy);


  if (!light)
    return miFALSE;
      
  result->r *= power;
  result->g *= power;
  result->b *= power;

  return miTRUE;
}
