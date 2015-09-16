
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lume_state.h>
#include <random.h>
#include <calc.h>
#include <gamma.h>
#include <lic.h>

#define NAME "Glass"
#define PROD PROD_LUMEMATTER

struct Glass_param
{
  struct soft_material si_default;

  miBoolean        tint_on;
  miBoolean        tint_diffuse_on;
  miBoolean        tint_solid_on;
  miColor          tint_solid_color;

  miBoolean        tblur_on;
  miScalar         tblur_spread;
  miInteger        tblur_samples;

  miBoolean        rblur_on;
  miScalar         rblur_spread;
  miInteger        rblur_samples;

  miBoolean        tedge_none_on;
  miBoolean        tedge_fresnel_on;
  miBoolean        tedge_custom_on;
  miScalar         tedge_custom_middle;
  miScalar         tedge_custom_edge;
  miScalar         tedge_custom_shift;

  miBoolean        sedge_none_on;
  miBoolean        sedge_custom_on;
  miScalar         sedge_custom_middle;
  miScalar         sedge_custom_edge;
  miScalar         sedge_custom_shift;

  miBoolean        translucency_none_on;
  miBoolean        translucency_fixed_on;
  miScalar         translucency_fixed_value;
  miBoolean        translucency_scale_on;
  miScalar         translucency_scale_value;

};



DLLEXPORT void Glass_init(miState *state, 
			   struct Glass_param *paras,
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




DLLEXPORT void Glass_exit(struct Glass_param *paras)
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





miBoolean lume_glass_trace_refraction(miColor *result, miState *state, 
				struct soft_material *m,
				struct Glass_param *paras, 
				miVector *dir)
{
  /* smooth surface case */
  if ( !(paras->tblur_on) ||
      (lume_did_multiray(state->parent)) )
    {
      return mi_trace_refraction(result, state, dir);
    }

  /* blurred surface case */
  if (paras->tblur_on)
    {
      miColor mix;
      int mix_count;
      double spread;
      miVector point;
      int i;
      miVector new_dir;

      ((struct lume_state *)state->user)->did_multiray = miTRUE;

      mix.r = mix.g = mix.b = mix.a = 0;
      mix_count = 0;


      spread = tan(paras->tblur_spread * PI / 180);
      mi_point_to_object(state, &point, &state->point);


      for (i = 0; i < paras->tblur_samples; i++)
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
	    hit = mi_trace_refraction(&tmp, state, &new_dir);

	    if (hit)
	      {
		mix.r += tmp.r;
		mix.g += tmp.g;
		mix.b += tmp.b;
		mix.a += tmp.a;
		mix_count++;
	      }
	  }

	}


      if (mix_count)
	{
	  result->r = mix.r / paras->tblur_samples;
	  result->g = mix.g / paras->tblur_samples;
	  result->b = mix.b / paras->tblur_samples;
	  result->a = mix.a / paras->tblur_samples;

	  return miTRUE;
	}

      else
	return miFALSE;
    }

  /* should never reach this point */
  return miFALSE;  
}




miBoolean lume_glass_tint_refraction(miColor *result, struct soft_material *m,
				struct Glass_param *paras)
{
  if (paras->tint_on)
    {
      if (paras->tint_diffuse_on)
	{
	  result->r *= m->diffuse.r;
	  result->g *= m->diffuse.g;
	  result->b *= m->diffuse.b;
	}
      if (paras->tint_solid_on)
	{
	  result->r *= paras->tint_solid_color.r;
	  result->g *= paras->tint_solid_color.g;
	  result->b *= paras->tint_solid_color.b;
	}
    }

  result->r *= m->transp;
  result->g *= m->transp;
  result->b *= m->transp;
  result->a *= m->transp;

  if ( (result->r) || (result->g) || (result->b))
    return miTRUE;
  else
    return miFALSE;

}

miBoolean lume_glass_trace_reflection(miColor *result, miState *state, 
				struct soft_material *m,
				struct Glass_param *paras, 
				miVector *dir)
{

  /* smooth surface case */
  if ( !(paras->rblur_on) ||
      (lume_did_multiray(state->parent)) )
    {
      if (m->notrace)
	 return mi_trace_environment(result, state, dir);
      else
	return mi_trace_reflection(result, state, dir);

    }

  /* blurred surface case */
  if (paras->rblur_on)
    {
      miColor mix;
      int mix_count;
      double spread;
      miVector point;
      int i;
      miVector new_dir;

      ((struct lume_state *)state->user)->did_multiray = miTRUE;

      mix_count = 0;
      mix.r = mix.g = mix.b = mix.a = 0;

      spread = tan(paras->rblur_spread * PI / 180);
      mi_point_to_object(state, &point, &state->point);


      for (i = 0; i < paras->rblur_samples; i++)
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
		
		mix_count++;
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

  /* should never reach this point */
  return miFALSE;
}






void lume_glass_refraction(miColor *result, miState *state,
			   struct soft_material *m,
			   struct Glass_param *paras,
			   double ior_in, double ior_out)
{
  miColor color;
  miVector dir;
  

  if (m->transp >0)
    {
      miScalar f;

      f = 1 - m->transp;

      result->r *= f;
      result->g *= f;
      result->b *= f;
      result->a *= f;

      if (!state->contour)
	{
	  int tir; /* total internal reflection */

	  miBoolean hit;

	  tir = !mi_refraction_dir(&dir, state, ior_in, ior_out);
	  
	  if (tir)
	    hit = lume_glass_trace_reflection(&color, state, m, paras, &dir);
	    
	  else
	    hit = lume_glass_trace_refraction(&color, state, m, paras, &dir);


	  /* if the ray hit something */
	  if (hit)
	    {
	      lume_glass_tint_refraction(&color, m, paras);

	      result->r += color.r;
	      result->g += color.g;
	      result->b += color.b;
	      result->a += color.a;
	    }
	}
    }
}






void lume_glass_reflection(miColor *result, miState *state,
			   struct soft_material *m,
			   struct Glass_param *paras)
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

	  hit = lume_glass_trace_reflection(&color, state, m, paras, &dir);

	  /* if the ray hit something */
	  if (hit)
	    {
	      result->r += m->reflect * color.r;
	      result->g += m->reflect * color.g;
	      result->b += m->reflect * color.b;
	      
	    }
	}
    }
}



void lume_glass_refraction_edge(miState *state, 
				struct soft_material *m, 
				struct Glass_param *paras,
				double ior_in, double ior_out)
{
  if (paras->tedge_custom_on)
    {
      double tr;

      tr = bias(sin(acos(state->dot_nd)), paras->tedge_custom_shift);

      m->transp *= tr * paras->tedge_custom_edge +
	(1.0-tr) * paras->tedge_custom_middle;

    }

  if (paras->tedge_fresnel_on)
    {
      double tr, t, r;

      tr = (1.0 - autogamma_scalar(
	    mi_fresnel_reflection(state, ior_in, ior_out)));

      t = m->transp;
      r = m->reflect;

      /* okay, this is gross. listen.

	 we want the trans% to be tr, and we want the refl% to be (1-tr).
	 but depending on what the meat-head user has dialed into the
	 material box, the trans% and refl% won't quite be
	 m->transp and m->reflect... Specifically, because refl% is
	 (1 - m->transp), if the user lowers m->transp, he'll unknowingly
	 raise refl%.

	 So we second guess him.  If he lowers m->transp, we'll compute
	 a m->reflect that cancels-out his damage.  (Now, of course,
	 if he alters m->reflect we want to honor his request.  This
	 mess is just so that he won't fuck-up refl% when he's trying
	 to modify trans%.)

	 Got that?  Good luck.

	 */

      m->transp  = t * tr;

      if (t * tr != 1.0)
	m->reflect = r * (1.0 - tr) / (1.0 - t * tr);
      else
	m->reflect = r;


    }
}


void lume_glass_shadow_edge(miState *state, 
			    struct soft_material *m, 
			    struct Glass_param *paras)
{
  if (paras->sedge_custom_on)
    {
      double tr;

      tr = bias(sin(acos(state->dot_nd)), paras->sedge_custom_shift);

      m->transp *= tr * paras->sedge_custom_edge +
	(1.0-tr) * paras->sedge_custom_middle;

    }

}



void lume_glass_translucency(miColor *result, miState *state,
			      struct soft_material *m,
			      struct Glass_param *paras)
{
  double t;

  if (paras->translucency_none_on)
    t = 0;

  if (paras->translucency_fixed_on)
    t = paras->translucency_fixed_value;

  if (paras->translucency_scale_on)
    t = m->transp * paras->translucency_scale_value;
  

  if (t >0)
    {
      miScalar f;
	
      f = 1 - t;

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

	for (i = 0; i < paras->si_default.n_light; i++)
	  {
	    miTag *light;
	    miColor light_color;
	    miVector dir;
	    miScalar dot_nl;
		    
	    light = &paras->si_default.light[paras->si_default.i_light + i];
	    
		    
	    if (mi_trace_light(&light_color, &dir, &dot_nl, state,
			       *light))
	      {
		color.r += light_color.r * (dot_nl) * m->diffuse.r;
		color.g += light_color.g * (dot_nl) * m->diffuse.g;
		color.b += light_color.b * (dot_nl) * m->diffuse.b;
			
	      }
	  }
		
	state->normal = real_normal;
		
	result->r += t * color.r;
	result->g += t * color.g;
	result->b += t * color.b;
      }
      
    }

}


	

DLLEXPORT miBoolean Glass(miColor *result, miState *state,
		    struct Glass_param *paras)
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
    lume_glass_shadow_edge(state, &m, paras);
  else
    lume_glass_refraction_edge(state, &m, paras, ior_in, ior_out);

  if (state->type == miRAY_SHADOW)
    return lume_glass_tint_refraction(result, &m,paras);

  mi_mtl_illumination(result, state, &m, &paras->si_default,
		      ior_in, ior_out);

  lume_glass_reflection(result, state, &m, paras);

  lume_glass_translucency(result, state, &m, paras);

  lume_glass_refraction(result, state, &m, paras, ior_in, ior_out);

  free_lume_state(state);

  return miTRUE;
}
