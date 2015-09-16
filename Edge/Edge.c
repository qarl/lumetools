
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <frac.h>
#include <calc.h>
#include <lic.h>

#define NAME "Edge"
#define PROD PROD_LUMEMATTER

struct Edge_param
{
  struct soft_material si_default;


  miScalar  profile_amount;
  miScalar  profile_blur;
  
  miBoolean profile_color_on;
  miScalar  profile_color_influence;

  miBoolean profile_noise_on;
  miScalar  profile_noise_influence;
  miScalar  profile_noise_roughness;
  miScalar  profile_noise_scale;

  miBoolean profile_relative_object_on;
  miBoolean profile_relative_world_on;

  miScalar  shadow_amount;

  miBoolean shadow_color_on;
  miScalar  shadow_color_influence;

  miBoolean shadow_noise_on;
  miScalar  shadow_noise_influence;
  miScalar  shadow_noise_roughness;
  miScalar  shadow_noise_scale;

  miBoolean shadow_relative_object_on;
  miBoolean shadow_relative_world_on;


  /* the following are defined by our init() routine */
  /* space has been allocated for them via dummy variables in the mi file */
  
  struct frac *profile_frac;
  struct frac *shadow_frac;
};




DLLEXPORT void Edge_init(miState *state, struct Edge_param *paras,
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

      if (paras->profile_noise_on)
	paras->profile_frac =
	  frac_create(2.0, (1.0 - paras->profile_noise_roughness) * 2.0, 4);
      else
	paras->profile_frac = NULL;

      if (paras->shadow_noise_on)
	paras->shadow_frac =
	  frac_create(2.0, (1.0 - paras->shadow_noise_roughness) * 2.0, 4);
      else
	paras->shadow_frac = NULL;

    }
}




DLLEXPORT void Edge_exit(struct Edge_param *paras)
{
  if (!paras)
    {
      /* global cleanup */
      lic_end(PROD);
    }

  else
    {
      /* instance cleanup */

      if (paras->profile_frac)
	frac_destroy(paras->profile_frac);

      if (paras->shadow_frac)
	frac_destroy(paras->shadow_frac);

    }

}


/********************************************************************

  Here begin the profile routines... the two sections should
  be unified... there's a lot of cut'n'paste here

  *******************************************************************/




double profile_noise_influence(double *potential, miState *state,
			       struct Edge_param *paras)
{
  double value;

  if (paras->profile_noise_on)
    {
      miVector point;

      if (paras->profile_relative_world_on)
	mi_point_to_world(state, &point, &state->point);

      if (paras->profile_relative_object_on)
	mi_point_to_object(state, &point, &state->point);

      mi_vector_div(&point, paras->profile_noise_scale);

      value = frac_scalar(paras->profile_frac, point);

      value *= paras->profile_noise_influence;
      *potential += paras->profile_noise_influence;

      return value;
    }

  else
    return 0;
}



double profile_color_influence(double *potential, struct soft_material *m,
			       struct Edge_param *paras)
{
  double value;

  if (paras->profile_color_on)
    {
      value = (m->diffuse.r + m->diffuse.g + m->diffuse.b) / 3;

      value *= paras->profile_color_influence;
      *potential += paras->profile_color_influence;

      return value;
    }

  else
    return 0;
}






miScalar profile_total_influence(miState *state, 
				 struct soft_material *m, 
				 struct Edge_param *paras)
{
  double pi;
  double influence;

  pi = influence = 0;

  influence += profile_noise_influence(&pi, state, paras);
  influence += profile_color_influence(&pi, m, paras);
  
  if (pi != 0.0)
    return influence / pi;
  else
    return 0;
  
}



#define K_PROF_AMOUNT 0.02

miScalar profile_effect(miState *state, 
			struct soft_material *m, 
			struct Edge_param *paras)
{
  return paras->profile_amount * K_PROF_AMOUNT *
    (1.0 + profile_total_influence(state, m, paras)) / 2.0;
}


#define K_BLUR_RANGE 0.1
#define K_BLUR_GAIN 0.8

void lume_edge_profile(miState *state, miScalar geo_dp,
		       struct soft_material *m,
		       struct Edge_param *paras)
{
  miScalar dp;

  miScalar e;
  float t;

  /* don't want to see the back side */
  if ((state->type == miRAY_REFRACT) ||
      (state->type == miRAY_TRANSPARENT))
    if ((state->parent) && (state->parent->shader == state->shader))
      {
	m->transp = 1;
	return;
      }
      


  dp = fabs(geo_dp);
  if (dp > 1)
    dp = 1;

  e = sin(acos(dp));

  e += profile_effect(state, m, paras);

  t = range_scale(e, 1.0 - K_BLUR_RANGE * paras->profile_blur,1,
		  0,1);
  t = gain(t, K_BLUR_GAIN);

  if (t > 1)
    t = 1;
  if (t < 0)
    t = 0;

  m->transp *= t;

}







/********************************************************************

  Here begin the shadow routines

  *******************************************************************/



double shadow_noise_influence(double *potential, miState *state,
			       struct Edge_param *paras)
{
  double value;

  if (paras->shadow_noise_on)
    {
      miVector point;

      if (paras->shadow_relative_world_on)
	mi_point_to_world(state, &point, &state->point);

      if (paras->shadow_relative_object_on)
	mi_point_to_object(state, &point, &state->point);

      mi_vector_div(&point, paras->shadow_noise_scale);

      value = frac_scalar(paras->shadow_frac, point);

      value *= paras->shadow_noise_influence;
      *potential += paras->shadow_noise_influence;

      return value;
    }

  else
    return 0;
}



double shadow_color_influence(double *potential, struct soft_material *m,
			       struct Edge_param *paras)
{
  double value;

  if (paras->shadow_color_on)
    {
      value = (m->diffuse.r + m->diffuse.g + m->diffuse.b) / 3;

      value *= paras->shadow_color_influence;
      *potential += paras->shadow_color_influence;

      return value;
    }

  else
    return 0;
}






miScalar shadow_total_influence(miState *state, 
				 struct soft_material *m, 
				 struct Edge_param *paras)
{
  double pi;
  double influence;

  pi = influence = 0;

  influence += shadow_noise_influence(&pi, state, paras);
  influence += shadow_color_influence(&pi, m, paras);
  
  if (pi != 0.0)
    return influence / pi;
  else
    return 0;
  
}





miScalar shadow_effect(miState *state, 
			struct soft_material *m, 
			struct Edge_param *paras)
{
  miScalar effect;

  effect = paras->shadow_amount *
    (1.0 + shadow_total_influence(state, m, paras)) / 2.0;

  if (effect < 0)
    effect = 0;

  return effect;
}





void lume_edge_illumination(miColor *result, miState *state, 
			    struct soft_material *m, 
			    struct Edge_param *paras, 
			    double ior_in, double ior_out,
			    miVector normal)
{
  int       n;              /* light counter */
  miTag     *light;         /* current light */
  miColor   color;          /* color from light source */
  miVector  dir;            /* direction towards light */
  miScalar  dot_nl;         /* dot prod of normal and dir*/
  miState   new_state;


  if (m->transp >= 1)
    {
      result->r = result->b = result->b = result->a = 0;
      return;
    }
  
  if ( (m->mode == 0) || state->contour)
    {
      *result   = m->diffuse;
      result->a = 1;
      return;
    }
  


  /* modify the state, so that we have a "fake" point */
  /* this is significantly more disgusting than it should have to be.
     I just don't get it. */

  {
    miVector new_point;

    new_point = normal;
    mi_vector_mul(&new_point, shadow_effect(state, m, paras));
    mi_vector_add(&new_point, &new_point, &state->point);

    new_state       = *state;
    new_state.point = new_point;
    new_state.pri   = 0;

  }



  *result = m->ambient;
  for (n=0; n < paras->si_default.n_light; n++) 
    {
      light = &paras->si_default.light[paras->si_default.i_light + n];

      if (mi_trace_light(&color, &dir, &dot_nl, &new_state, (miTag) *light))
	{
	  /* because pri = 0, we need to compute dot_nl for ourselves */
	  dot_nl = mi_vector_dot(&dir, &state->normal);

	  result->r += dot_nl * m->diffuse.r * color.r;
	  result->g += dot_nl * m->diffuse.g * color.g;
	  result->b += dot_nl * m->diffuse.b * color.b;

	  if (m->mode == 2)
	    {
	      miScalar ns;

	      ns = mi_phong_specular(m->shiny, state, &dir);
	      result->r += ns * m->specular.r * color.r;
	      result->g += ns * m->specular.g * color.g;
	      result->b += ns * m->specular.b * color.b;
	    }

	  if (m->mode == 3)
	    {
	      miScalar ns;
	      miScalar kls;

	      mi_fresnel_specular(&ns, &kls, m->shiny, state, &dir,
				  ior_in, ior_out);

	      result->r += ns * (m->specular.r + kls * (1.0 - m->specular.r))
		* color.r;
	      result->g += ns * (m->specular.g + kls * (1.0 - m->specular.g))
		* color.g;
	      result->b += ns * (m->specular.b + kls * (1.0 - m->specular.b))
		* color.b;
	    }


	}
    }



  result->a = 1;
}





DLLEXPORT miBoolean Edge(miColor *result, miState *state,
		    struct Edge_param *paras)
{
  
  struct soft_material m;

  miScalar ior_in, ior_out;
  miScalar geo_dp;           /* geometry dp, before bumping */
  miVector geo_normal;       /* geometry n, before bumping */

  if (state->type == miRAY_SHADOW)
    {
      if (!mi_mtl_is_casting_shadow(state, &paras->si_default))
	return miFALSE;
    }
  else
    mi_mtl_refraction_index(state, &paras->si_default, &ior_in, &ior_out);

  geo_normal = state->normal;
  geo_dp     = state->dot_nd;

  m = paras->si_default;
  
  mi_mtl_textures(state, &m, &paras->si_default,
		  &state->normal, &state->dot_nd);

  mi_mtl_static_blur(state, &m);

  lume_edge_profile(state, geo_dp, &m, paras);

  if (state->type == miRAY_SHADOW)
    {
      /* no coloring the shadows, please! */
      m.diffuse.r = m.diffuse.g = m.diffuse.b = 1;
      return mi_mtl_compute_shadow(result, &m);
    }

  if (paras->shadow_amount != 0)
    lume_edge_illumination(result, state, &m, paras,
			   ior_in, ior_out, geo_normal);
  else
    mi_mtl_illumination(result, state, &m, &paras->si_default,
			ior_in, ior_out);

  mi_mtl_reflection(result, state, &m);
  
  mi_mtl_refraction(result, state, &m, ior_in, ior_out);

  return miTRUE;
		      
}
