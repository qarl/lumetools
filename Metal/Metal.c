
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lume_state.h>
#include <random.h>
#include <lic.h>

#define NAME "Metal"
#define PROD PROD_LUMEMATTER

struct Metal_param
{
  struct soft_material si_default;

  miBoolean reflect_clear_on;
  miBoolean reflect_specular_on;
  miBoolean reflect_solid_on;
  miColor reflect_solid_color;


  miBoolean surface_blurred_on;
  miScalar surface_blurred_spread;
  miInteger surface_blurred_samples;
};





DLLEXPORT void Metal_init(miState *state, 
			   struct Metal_param *paras,
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




DLLEXPORT void Metal_exit(struct Metal_param *paras)
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





miBoolean lume_metal_trace_reflection(miColor *result, miState *state, 
				struct soft_material *m,
				struct Metal_param *paras, 
				miVector *dir)
{
  /* smooth surface case */
  if ( !(paras->surface_blurred_on) ||
      (lume_did_multiray(state->parent)) )
    {
      miBoolean hit;

      if (m->notrace)
	hit = mi_trace_environment(result, state, dir);
      else
	hit = mi_trace_reflection(result, state, dir);
      
      return hit;
    }

  /* blurred surface case */
  if (paras->surface_blurred_on)
    {
      miColor mix;
      int mix_count;

      double spread;
      miVector point;
      int i;
      miVector new_dir;

      ((struct lume_state *)state->user)->did_multiray = miTRUE;

      mix.r = mix.g = mix.b = 0;
      mix_count = 0;

      spread = tan(paras->surface_blurred_spread * PI / 180);
      mi_point_to_object(state, &point, &state->point);


      for (i = 0; i < paras->surface_blurred_samples; i++)
	{
	  /* perturb the direction */
	  {
	    miVector r;

	    r = lume_random_4x3(point,i);

	    /* Make it point in same direction as dir */
	    if (mi_vector_dot(&r,dir) < 0)
	      {
		r.x *= -1.0;
		r.y *= -1.0;
		r.z *= -1.0;
	      }

	    new_dir.x = dir->x + spread * r.x;
	    new_dir.y = dir->y + spread * r.y;
	    new_dir.z = dir->z + spread * r.z;

	    mi_vector_normalize(&new_dir);
	  }

	  
	  /* shoot a ray */
	  {
	    miColor tmp;
	    int hit;

	    tmp = *result;
	    if (m->notrace)
	      hit = mi_trace_environment(&tmp, state, &new_dir);
	    else
	      hit = mi_trace_reflection(&tmp, state, &new_dir);
	      
	    if (hit)
	      {
		mix.r += tmp.r;
		mix.g += tmp.g;
		mix.b += tmp.b;
		mix_count ++;
	      }
	  }

	}

      if (mix_count)
	{
	  result->r = mix.r / mix_count;
	  result->g = mix.g / mix_count;
	  result->b = mix.b / mix_count;
	  return miTRUE;
	}
      else
	return miFALSE;
    }

  /* should never reach here */
  return miFALSE;

}






void lume_metal_color_reflection(miColor *result, 
				 struct soft_material *m,
				 struct Metal_param *paras)
{

  if (paras->reflect_specular_on)
    {
      result->r *= m->specular.r;
      result->g *= m->specular.g;
      result->b *= m->specular.b;
    }

  if (paras->reflect_solid_on)
    {
      result->r *= paras->reflect_solid_color.r;
      result->g *= paras->reflect_solid_color.g;
      result->b *= paras->reflect_solid_color.b;
    }
}





void lume_metal_reflection(miColor *result, miState *state,
			   struct soft_material *m,
			   struct Metal_param *paras)
{
  miColor color;
  miVector dir;

  if (m->reflect >0)
    {
      miScalar f;

      f = 1 - m->reflect;

      result->r *= f;
      result->g *= f;
      result->b *= f;

      if (!state->contour)
	{
	  miBoolean hit;

	  mi_reflection_dir(&dir, state);

	  hit = lume_metal_trace_reflection(&color, state, m, paras, &dir);

	  /* if the ray hit something */
	  if (hit)
	    {
	      lume_metal_color_reflection(&color, m, paras);

	      result->r += m->reflect * color.r;
	      result->g += m->reflect * color.g;
	      result->b += m->reflect * color.b;
	      
	    }
	}
    }

}



DLLEXPORT miBoolean Metal(miColor *result, miState *state,
		    struct Metal_param *paras)
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
  
    
  m = paras->si_default;
  
  mi_mtl_textures(state, &m, &paras->si_default, 
		  &state->normal, &state->dot_nd);

  mi_mtl_static_blur(state, &m);

  if (state->type == miRAY_SHADOW)
    return mi_mtl_compute_shadow(result, &m);

  mi_mtl_illumination(result, state, &m, &paras->si_default,
		      ior_in, ior_out);

  lume_metal_reflection(result, state, &m, paras);

  mi_mtl_refraction(result, state, &m, ior_in, ior_out);

  free_lume_state(state);

  return miTRUE;
}

