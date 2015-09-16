
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>
#include <frac.h>
#include <calc.h>
#include <color.h>

#define NAME "Landscape"
#define PROD PROD_LUMELANDSCAPE

struct Landscape_param
{
  struct soft_color si_default;

  miScalar blur;
  miBoolean relative_object_on;
  miBoolean relative_world_on;

  struct previewcolor_Landscape_param
  {
    miBoolean none;
    miBoolean red;
    miBoolean yellow;
    miBoolean green;
    miBoolean blue;
    miBoolean violet;
  } previewcolor;

  struct image_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miBoolean alpha;
    miBoolean red;
    miBoolean green;
    miBoolean blue;
  } image;

  struct height_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miBoolean upsidedown;
    miScalar  height;
    miScalar  spread;
  } height;

  struct slope_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miBoolean upsidedown;
    miScalar  angle;
  } slope;

  struct positional_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miScalar  size;
    miScalar  splotch;
    miScalar  coverage;
    miScalar  yscale;
  } positional;

  struct surface_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miScalar  size;
    miScalar  splotch;
    miScalar  coverage;
    miScalar  yscale;
  } surface;

  struct stain_Landscape_param
  {
    miBoolean on;
    miScalar  influence;
    miScalar  thickness;
  } stain;


  /* the following are defined by our init() routine */
  /* space has been allocated for them via dummy variables in the mi file */

  struct frac *positional_frac;
  struct frac *surface_frac;
};




DLLEXPORT void Landscape_init(miState *state, struct Landscape_param *paras,
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
      
      if (paras->positional.on)
	paras->positional_frac = 
	  frac_create(2.0, (1.0 - paras->positional.splotch) * 2.0, 10);
      else
	paras->positional_frac = NULL;

      if (paras->surface.on)
	paras->surface_frac = 
	  frac_create(2.0, (1.0 - paras->surface.splotch) * 2.0 , 10);
      else
	paras->surface_frac = NULL;
	
    }
}




DLLEXPORT void Landscape_exit(struct Landscape_param *paras)
{
  if (!paras)
    {
      /* global cleanup */
      lic_end(PROD);
    }

  else
    {
      /* instance cleanup */

      if (paras->positional_frac)
	frac_destroy(paras->positional_frac);

      if (paras->surface_frac)
	frac_destroy(paras->surface_frac);
    }

}





double image_influence(double *potential,
		       struct image_Landscape_param *image_paras,
		       miColor *color)
{
  double value;

  value = 0;

  if ((image_paras->on) && (image_paras->influence))
    {
      if (image_paras->alpha)
	value += (2.0 * color->a - 1.0);

      if (image_paras->red)
	value += (2.0 * color->r - 1.0);

      if (image_paras->green)
	value += (2.0 * color->g - 1.0);

      if (image_paras->blue)
	value += (2.0 * color->b - 1.0);

      value *= image_paras->influence;

      *potential += image_paras->influence;

      return value;
    }

  else
    return 0;
}






double height_influence(double *potential,
			struct height_Landscape_param *height,
			miVector point)
{
  double value;

  if ((height->on) && (height->influence))
    {
      value = range_scale(point.y,
			  height->height - height->spread, 
			  height->height + height->spread, 
			  -1, 1);

      value *= height->influence;
      *potential += height->influence;

      if (height->upsidedown)
	value *= -1.0;

      return value;
    }

  else
    return 0;
}



double slope_influence(double *potential,
		       struct slope_Landscape_param *slope_paras,
		       miVector normal)
{
  double value;

  if ((slope_paras->on) && (slope_paras->influence))
    {
      double angle;

      angle = acos(-normal.y)/PI*180.0 + slope_paras->angle -180.0;

      value = range_scale(angle, -90, 90, -1, 1);

      value *= slope_paras->influence;

      *potential += slope_paras->influence;

      if (slope_paras->upsidedown)
	value *= -1.0;


      return value;
    }

  else
    return 0;
}



/* Compute the positional noise component */
double positional_influence(double *potential,
			    struct positional_Landscape_param *pos,
			    struct frac *frac,
			    miVector point)
{
  double value;

  if ((pos->on) && (pos->influence != 0.0))
    {
      point.y /= pos->yscale;
      mi_vector_div(&point, pos->size);

      value = (2.0 * frac_scalar(frac, point)) + pos->coverage;

      value *= pos->influence;
      *potential += pos->influence;
      
      return value;
    }

  else
    return 0;
}



/* Compute the surface noise component */
double surface_influence(double *potential,
			 struct surface_Landscape_param *surface_paras,
			 struct frac *frac,
			 miVector normal)
{
  double value;

  if ((surface_paras->on) && (surface_paras->influence != 0.0))
    {
      normal.y /= surface_paras->yscale;
      mi_vector_div(&normal, sin(surface_paras->size * PI/180.0));

      value = (2.0 * frac_scalar(frac, normal)) + surface_paras->coverage;

      value *= surface_paras->influence;
      *potential += surface_paras->influence;
      
      return value;
    }

  else
    return 0;
}


/* Compute the stain component */
double stain_influence(double *potential,
		       struct stain_Landscape_param *stain_paras,
		       miState *state)
{
  double value;

  if ((stain_paras->on) && (stain_paras->influence != 0.0))
    {
      double d;
      
      if ( (state->parent) &&
	   (is_stained_lume_state(state->parent)) )
	d = state->dist;
      else
	d = 0;

      value = range_scale(d, 0, stain_paras->thickness,
			   -1,1);

      value *= stain_paras->influence;
      *potential += stain_paras->influence;
      
      return value;
    }

  else
    return 0;
}







/* tabulate the total influence.  Return value in range [-1, 1] */
double total_influence(miColor *color, miState *state,
			struct Landscape_param *paras)
{
  double influence;
  double pi;
  miVector point;
  miVector normal;


  if (paras->relative_world_on)
    {
      mi_point_to_world(state, &point, &state->point);
      mi_vector_to_world(state, &normal, &state->normal);
    }
  if (paras->relative_object_on)
    {
      mi_point_to_object(state, &point, &state->point);
      mi_vector_to_object(state, &normal, &state->normal);
    }

  mi_vector_normalize(&normal);

  pi = influence = 0;

  influence += height_influence(&pi, &paras->height,
				point);

  influence += slope_influence(&pi, &paras->slope,
			       normal);

  influence += positional_influence(&pi, &paras->positional,
				    paras->positional_frac,
				    point);

  influence += surface_influence(&pi, &paras->surface,
				 paras->surface_frac,
				 normal);

  influence += image_influence(&pi, &paras->image,
			       color);

  influence += stain_influence(&pi, &paras->stain,
			       state);


  if (pi != 0.0)
    return influence / pi;
  else
    return 0;

}



double influence_to_alpha(struct Landscape_param *paras,
		   double influence)
{
  return range_scale(influence, 
		     -paras->blur, paras->blur,
		     0,1);
}





void previewcolor(miColor *color, 
		  struct previewcolor_Landscape_param *pc_paras)
{
  double alpha_save;

  if (pc_paras->none)
    return;


  alpha_save = color->a;

  if (pc_paras->red)
    *color = red;
  if (pc_paras->yellow)
    *color = yellow;
  if (pc_paras->green)
    *color = green;
  if (pc_paras->blue)
    *color = blue;
  if (pc_paras->violet)
    *color = violet;

  color->a = alpha_save;

}





DLLEXPORT miBoolean Landscape(miColor *result, miState *state,
		    struct Landscape_param *paras)
{
  double influence;

  soft_color(result, state, &paras->si_default);

  influence = total_influence(result, state, paras);

  result->a = influence_to_alpha(paras, influence);

  previewcolor(result, &paras->previewcolor);

  return miTRUE;
}


