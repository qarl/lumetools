
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "Translucency"
#define PROD PROD_LUMELIGHT

struct Translucency_param
{
  struct soft_material si_default;

  miBoolean dum;
};




DLLEXPORT void Translucency_init(miState *state, 
			   struct Translucency_param *paras,
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




DLLEXPORT void Translucency_exit(struct Translucency_param *paras)
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



void lume_mtl_transluncency(miColor *result, miState *state,
			    struct soft_material *m,
			    struct soft_material *paras)
{
  if (m->transp >0)
    {
      miScalar f;

      f = 1 - m->transp;

      result->r *= f;
      result->g *= f;
      result->b *= f;
      result->a *= f;

      if (state->contour)
	return;

      {
	miColor color;
	int i;
	miVector real_normal;

	color.r = color.g = color.b = 0;
	
	real_normal = state->normal;
	state->normal.x *= -1.0;
	state->normal.y *= -1.0;
	state->normal.z *= -1.0;

	for (i = 0; i < paras->n_light; i++)
	  {
	    miTag *light;
	    miColor light_color;
	    miVector dir;
	    miScalar dot_nl;

	    light = &paras->light[paras->i_light + i];


	    if (mi_trace_light(&light_color, &dir, &dot_nl, state,
			       *light))
	      {
		color.r += light_color.r * (dot_nl) * m->diffuse.r;
		color.g += light_color.g * (dot_nl) * m->diffuse.g;
		color.b += light_color.b * (dot_nl) * m->diffuse.b;

	      }
	  }

	state->normal = real_normal;

	result->r += m->transp * color.r;
	result->g += m->transp * color.g;
	result->b += m->transp * color.b;
      }

    }

}


DLLEXPORT miBoolean Translucency(miColor *result, miState *state,
		    struct Translucency_param *paras)
{
  struct soft_material m;

  miScalar ior_in, ior_out;

  if (state->type == miRAY_SHADOW)
    {
      if (!mi_mtl_is_casting_shadow(state, &paras->si_default))
	return miFALSE;
    }
  else
    mi_mtl_refraction_index(state, &paras->si_default, &ior_in, &ior_out);
    
  m = paras->si_default;

  mi_mtl_textures(state, &m, &paras->si_default, 
		  &state->normal, &state->dot_nd);

  mi_mtl_static_blur(state, &m);

  if (state->type == miRAY_SHADOW)
    return mi_mtl_compute_shadow(result, &m);

  mi_mtl_illumination(result, state, &m, &paras->si_default,
		      ior_in, ior_out);

  mi_mtl_reflection(result, state, &m);

  lume_mtl_transluncency(result, state, &m, &paras->si_default);

  return miTRUE;
}
