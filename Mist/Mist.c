
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <lic.h>

#define NAME "Mist"
#define PROD PROD_LUMELANDSCAPE

#define BIG_NUMBER 1000000
#define K_DENSITY 0.98
#define K_LAYERING_DENSITY 0.5

struct Mist_param
{
  miScalar transparency;
  miBoolean alpha_on;

  miBoolean color_solid_on;
  miColor color_solid_color;
  miBoolean color_map_on;
  miTag color_map_filename;

  miBoolean falloff_linear_on;
  miScalar falloff_linear_start;
  miScalar falloff_linear_end;

  miBoolean falloff_realistic_on;
  miScalar falloff_realistic_density;

  miBoolean falloff_custom_on;
  miScalar falloff_custom_start;
  miScalar falloff_custom_mid;
  miScalar falloff_custom_end;

  miBoolean layering_on;
  miScalar layering_baseline;
  miScalar layering_height;
};




DLLEXPORT void Mist_init(miState *state, struct Mist_param *paras,
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

      if ((paras->color_map_on) && (paras->color_map_filename == NULL))
	mi_warning("[Mist] Color map invalid. Using solid white.\n");
     }
}




DLLEXPORT void Mist_exit(struct Mist_param *paras)
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

void lume_lookup_color_texture(miColor *fog_color, miState *state,
			  miTag color_map_filename,
			  miVector *map_point)
{

  if (color_map_filename)

    mi_lookup_color_texture(fog_color, state,
			    color_map_filename,
			    map_point);
  else
      fog_color->r = fog_color->g = fog_color->b = 1;

}
	



DLLEXPORT miBoolean Mist(miColor *result, miState *state,
		    struct Mist_param *paras)
{
  double thickness, weight;
  miColor fog_color;

 if (state->type >=miRAY_LIGHT)
   return miTRUE;


  /* Below we determine the vitual "thickness" of the fog we've
     traveled through... Thickness can be thought of as the number
     of fog particles we've struck. */

  {
    if (paras->layering_on)
      {
	double s, h, l, k, d;  /* main variables */
	miVector dir, org;

	mi_point_to_world(state, &org, &state->org);
	mi_vector_to_world(state, &dir, &state->dir);

	k = 1.0 / paras->layering_height;
	l = state->dist;
	s = k * dir.y;
	h = k * (org.y - paras->layering_baseline);
	d = K_LAYERING_DENSITY;

	if (s == 0)
	  s = 1.0 / BIG_NUMBER;  /* I know, I'm sorry, I won't
				    do it again. */


	/* Do the algorithm.  
	   (See "Tale of a Shader Company", Jan 24, 1997) */

	{
	  double t;

	  t = pow(d,h) / s / log(d);
	  

	  /* infinite distance case */
	  if (l == 0) 
	    {
	      if (s < 0)
		thickness = BIG_NUMBER;
	      else
		thickness = -t;
	    }

	  /* finite distance case */
	  else
	    thickness = t * (pow(d,s*l) - 1.0);
	}

      }



    /* else no layering, assume constant density */

    else
      thickness = state->dist;


    /* a hack to deal with infinity (==0) */
    if (thickness == 0)
      thickness = BIG_NUMBER;   /* I lied. */
  }



  /* Below we deal with the falloff.
     Given a "thickness" of fog that the ray has travelled through,
     compute a "weight" for the fog coloration. */
  {

    if (paras->falloff_linear_on)
      weight = range_scale(thickness,
			   paras->falloff_linear_start,
			   paras->falloff_linear_end,
			   0,1);


    if (paras->falloff_realistic_on)
      weight = 1.0 - pow(K_DENSITY, 
		   paras->falloff_realistic_density * thickness);


    if (paras->falloff_custom_on)
      {
	double scaled_thick;
	double scaled_mid;

	scaled_thick = range_scale(thickness,
				   paras->falloff_custom_start,
				   paras->falloff_custom_end,
				   0,1);

	scaled_mid   =  range_scale(paras->falloff_custom_mid,
				    paras->falloff_custom_start,
				    paras->falloff_custom_end,
				    0,1);

	weight = bias(scaled_thick, scaled_mid);
      
      }


    /* Do not stray, my little friend */
    weight = clamp(weight, 0, 1);

    /* Bring within transparency setting */
    weight = range_scale(weight, 0, 1, 0, 1.0 - paras->transparency);
  }



  
  /* Below deal with determining the color of the fog */
  {
    if (paras->color_solid_on)
      fog_color = paras->color_solid_color;

    if (paras->color_map_on)
      {
	miVector map_point;

	/* Determine the map coord point */
	{
	  miVector dir;

	  mi_vector_to_world(state, &dir, &state->dir);

	  map_point.x = atan2(dir.z,dir.x) / (2.0 * PI);
	  map_point.y = -acos(dir.y)/PI;
	  map_point.z = 0;
	}

	lume_lookup_color_texture(&fog_color, state,
				paras->color_map_filename,
				&map_point);
	
      }
  }




  /* Mix the incoming color with the fog's color according to
     the weight */

  result->r = weight * fog_color.r  +  (1.0 - weight) * result->r;
  result->g = weight * fog_color.g  +  (1.0 - weight) * result->g;
  result->b = weight * fog_color.b  +  (1.0 - weight) * result->b;

  if (paras->alpha_on)
    result->a = weight  +  (1.0 - weight) * result->a;


  return miTRUE;
}
