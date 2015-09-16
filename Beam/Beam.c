
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <gamma.h>
#include <lic.h>

#define NAME "Beam"
#define PROD PROD_LUMELIGHT

#define BIG_NUMBER 10000000

struct Beam_param
{
  miColor  particle_color;
  miScalar brightness;

  int      i_lights;
  int      n_lights;
  miTag    *lights;
};

struct preview_light
{
  miVector position;
  miColor  color;
};

struct preview_light preview_lights[] = {
  {{2.6, 2.0, -10}, {1,1,1}},
  {{-2.6, 1.0, -10}, {0.2,0.6,2}},
  {{2.4, -1.0, -10}, {2,1.2,0.2}}
};





DLLEXPORT void Beam_init(miState *state, struct Beam_param *paras,
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




DLLEXPORT void Beam_exit(struct Beam_param *paras)
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



/* compute the amount of light reflected back along the ray.
   (For derivation, see "Tale of a Shader Company", Jan 26 1997) */


double compute_lighting_point(miVector eye_dir, miVector eye_org,
			      miVector light_org, double l)
{
  miVector m,b;  /* The slope and intercept of the ray */
  double A,B,C;  /* Cooefficients for the integral */
  double t;      /* term which reappears in computation */
  double i;

  /* set the b (intercept) vector */
  mi_vector_sub(&b, &eye_org, &light_org);

  /* set the m (slope) vector */
  m = eye_dir;


  /* Set the cooefficients */
  
  /* m is a unit vector, A happens to be it's length, nitwit */
  
  A = 1;
  A =      m.x*m.x + m.y*m.y + m.z*m.z  ;


  B = 2.0 * (m.x*b.x + m.y*b.y + m.z*b.z) ;

  C =      b.x*b.x + b.y*b.y + b.z*b.z  ;

  
  /* Evaluate the definite integral */
  {
    double tmp;
    
    tmp = 4*A*C - B*B;

    if (tmp <= 0)
      t = 0.0001;
    else
      t = sqrt(tmp);
  }

  i = 2/t * ( atan((B+2*A*l)/t) - atan((B)/t));


  return i * 0.01;

}

double compute_lighting_inf(double l)
{
  /* this case is much easier than the other */

  return l * 0.004;
}


miColor compute_lighting_preview(miState *state,
				 miVector eye_dir, miVector eye_org)
{
  double l;
  double strength;
  miVector preview_light;
  miColor color;
  int i;

  color.r = color.g = color.b = 0;

  for (i = 0; i < sizeof(preview_lights) / sizeof(struct preview_light); i++)
    {

      if (state->dist == 0) /* is infinite */
	l = BIG_NUMBER;
      else
	l = state->dist;

      strength = compute_lighting_point(eye_dir, eye_org, 
					preview_lights[i].position, l);

      color.r += strength * preview_lights[i].color.r;
      color.g += strength * preview_lights[i].color.g;
      color.b += strength * preview_lights[i].color.b;
    }

  return color;
}


miColor compute_lighting(miState *state, miTag light,
			miVector eye_dir, miVector eye_org)
{
  miVector light_org, light_dir;
  struct soft_spot *light_paras;
  double strength;
  miColor lighting;
  double l;


  mi_light_info(light, &light_org, &light_dir, (void **)&light_paras);

  mi_point_to_world(state, &light_org, &light_org);

  if (state->dist == 0) /* is infinite */
    l = BIG_NUMBER;
  else
    l = state->dist;

  if ((light_dir.x == 0) && (light_dir.y == 0) && (light_dir.z == 0))
    strength = compute_lighting_point(eye_dir, eye_org, light_org, l);
  else
    strength = compute_lighting_inf(l);


  lighting.r = strength * light_paras->color.r;
  lighting.g = strength * light_paras->color.g;
  lighting.b = strength * light_paras->color.b;

  return lighting;
}


DLLEXPORT miBoolean Beam(miColor *result, miState *state,
		    struct Beam_param *paras)
{
  int i;
  miVector dir, org;
  miColor beam_color;

  if (state->type >=miRAY_LIGHT)
    return miTRUE;


  mi_vector_to_world(state, &dir, &state->dir);
  mi_point_to_world(state, &org, &state->org);

  beam_color.r = beam_color.b = beam_color.g = 0;

  if (state->options->preview_mode)
    {
      beam_color = compute_lighting_preview(state, dir, org);
    }

  else
    for (i = 0 ; i < paras->n_lights; i++)
    {
      miTag  light;
      struct soft_light *light_paras;
      miColor lighting;

      light = paras->lights[paras->i_lights + i];

      lighting = compute_lighting(state, light, dir, org);

      beam_color.r += lighting.r;
      beam_color.g += lighting.g;
      beam_color.b += lighting.b;
    }

  beam_color = autogamma_color(beam_color);

  result->r += beam_color.r * paras->brightness * paras->particle_color.r;
  result->g += beam_color.g * paras->brightness * paras->particle_color.g;
  result->b += beam_color.b * paras->brightness * paras->particle_color.b;

  /* the integral doesn't converge for rays that pass through the center
     of the light.  This causes us grief with distant flickering.  So
     we clip here. */

#define CLIP 1.0

  if (result->r > CLIP)
    result->r = CLIP;
  if (result->g > CLIP)
    result->g = CLIP;
  if (result->b > CLIP)
    result->b = CLIP;


  return miTRUE;
}
