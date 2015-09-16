
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lume_state.h>
#include <gamma.h>
#include <lic.h>

#define NAME "WaterSurface"
#define PROD PROD_LUMEWATER

struct WaterSurface_param
{
  struct soft_material si_default;

  miBoolean look_in_on;
  miBoolean look_out_on;

  miScalar  ior;

  miBoolean stain_on;
};





DLLEXPORT void WaterSurface_init(miState *state, 
			   struct WaterSurface_param *paras,
			   miBoolean *req)
{
  if (req)
    {
      /* global shader init here, paras == NULL */

      lic(state, NAME, PROD);

      *req = miTRUE;
    }

  else
    {
      /* instance init here, paras valid */
    }
}




DLLEXPORT void WaterSurface_exit(struct WaterSurface_param *paras)
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


void lume_water_refraction_index(miState *state, 
				 struct WaterSurface_param *paras, 
				 miScalar *ior_in, miScalar *ior_out)
{
  int inside;
  miState *s;

  if (paras->look_in_on)
    inside = 0;
  if (paras->look_out_on)
    inside = 1;

  for (s = state->parent; s ; s = s->parent)
    {
      if ((s->shader == state->shader) &&
	  ( (s->type == miRAY_REFRACT) ||
	    (s->type == miRAY_TRANSPARENT)))
	inside++;
    }

  if (inside & 1)
    {
      *ior_in  = paras->ior;
      *ior_out = 1.0;
    }
  else
    {
      *ior_in  = 1.0;
      *ior_out = paras->ior;
    }

  state->ior    = *ior_out;
  state->ior_in = *ior_in;

}



void  lume_water_fresnel(miState *state,
			 struct soft_material *m, 
			 miScalar ior_in, miScalar ior_out)
{
  double f;
  double t,r,k;

  f = autogamma_scalar(mi_fresnel_reflection(state, ior_in, ior_out));
  f = fabs(f);

  t = m->transp;
  r = m->reflect;

  m->transp  = t * (1.0 - f);

  /* k is a scaling factor to correct R when the user monkeys with m->transp.
   * See the Glass source for more details.
   */

  if (m->transp == 1.0)
    k = 1;
  else
    k = f / (1.0 - m->transp);


  m->reflect = r * k;

}



DLLEXPORT miBoolean WaterSurface(miColor *result, miState *state,
		    struct WaterSurface_param *paras)
{
  struct soft_material m;
  miScalar ior_in, ior_out;


  if (!init_lume_state(state))
    mi_fatal("Cannot allocate lume_state\n");


  if (state->type == miRAY_SHADOW)
      {
	if (!mi_mtl_is_casting_shadow(state, &paras->si_default))
	  return miFALSE;
      }
  else
    mi_mtl_refraction_index(state, &paras->si_default, &ior_in, &ior_out);

  lume_water_refraction_index(state, paras, &ior_in, &ior_out);
    
  m = paras->si_default;
  
  mi_mtl_textures(state, &m, &paras->si_default, 
		  &state->normal, &state->dot_nd);

  mi_mtl_static_blur(state, &m);

  lume_water_fresnel(state, &m, ior_in, ior_out);

  if (state->type == miRAY_SHADOW)
    return mi_mtl_compute_shadow(result, &m);

  mi_mtl_illumination(result, state, &m, &paras->si_default,
		      ior_in, ior_out);

  mi_mtl_reflection(result, state, &m);


  /* stain transparency rays only */
  if (paras->stain_on)
    ((struct lume_state *)state->user)->stained = miTRUE;

  mi_mtl_refraction(result, state, &m, ior_in, ior_out);

  if (paras->stain_on)
    ((struct lume_state *)state->user)->stained = miFALSE;


  free_lume_state(state);

  return miTRUE;
}
