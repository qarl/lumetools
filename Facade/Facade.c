
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "Facade"
#define PROD PROD_LUMELANDSCAPE

struct Facade_param
{
  struct soft_color si_default;

  miScalar  size;

  miBoolean rotation_cylindrical_on;
  miBoolean rotation_spherical_on;
};


miVector zero = {0,0,0};
miVector up   = {0,1,0};


DLLEXPORT void Facade_init(miState *state, struct Facade_param *paras,
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




DLLEXPORT void Facade_exit(struct Facade_param *paras)
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






/* lume_plane_ray_intersection -
     compute the intersection of a plane and a ray.  
     Return:

       0 if parallel (no intersection), or

       distance from ray_point to intersection otherwise (may
       be negative if behind ray_point)
*/

miScalar lume_plane_ray_intersection(miVector *intersection,
				      miVector *plane_point, 
				      miVector *plane_dir,
				      miVector *ray_point,
				      miVector *ray_dir)
{
  miScalar q;
  miScalar t;

  q = mi_vector_dot(plane_dir, ray_dir);

  if (fabs(q) < 0.0001)
    return 0;  /* parallel */
    
  t = (mi_vector_dot(plane_point, plane_dir) -
       mi_vector_dot(plane_dir, ray_point)) / q;

  *intersection = *ray_dir;

  mi_vector_mul(intersection, t);
  mi_vector_add(intersection, intersection, ray_point);

  return t;

}





miBoolean Facade_tex_coord(miState *state, miScalar size, 
			       miBoolean cylindrical,
			       miVector *tex_point)
{
  miVector facing, facade_center, observer_center;
  miVector intersection;
  miVector fx, fy, fz;

  /* first - determine the center of the facade,
             the center of the observer, 
             and the direction of the facade */
  {

    mi_point_from_object(state, &facade_center, &zero);

    observer_center = state->org;

    mi_vector_sub(&facing, &observer_center, &facade_center);

    if (cylindrical)
      {
	miVector fix;

	mi_vector_to_object(state, &fix, &facing);
	fix.y = 0;
	mi_vector_from_object(state, &facing, &fix);
      }

    mi_vector_normalize(&facing);
  }



  /* second - compute the intersection of the ray and the facade */

  {
    miScalar l;

    l = lume_plane_ray_intersection(&intersection, 
				       &facade_center, &facing,
				       &state->point, &state->dir);



    if (l <= 0)  /* no intersection */
      return miFALSE;

  }



  /* third - build a coordinate system for the texture */
  {
    miVector object_up;

    fz = facing;

    mi_vector_from_object(state, &object_up, &up);
    mi_vector_prod(&fx, &object_up, &fz);
    mi_vector_normalize(&fx);
 
    mi_vector_prod(&fy, &fz, &fx);
    mi_vector_normalize(&fy);
 
  }


  
  /* forth - convert the intersection to the texture coordinate system */
  {
    mi_vector_sub(&intersection, &intersection, &facade_center);

    tex_point->x = mi_vector_dot(&intersection, &fx);
    tex_point->y = mi_vector_dot(&intersection, &fy);
    tex_point->z = mi_vector_dot(&intersection, &fz);

    mi_vector_div(tex_point, size);

    tex_point->x += 0.5;
    tex_point->y += 0.5;
  }
 


  
  return miTRUE;
}



DLLEXPORT miBoolean Facade(miColor *result, miState *state,
		    struct Facade_param *paras)
{
  miVector tex_point;
  miBoolean h;

  h = Facade_tex_coord(state, paras->size, 
		       paras->rotation_cylindrical_on, &tex_point);

  /* scale for aspect ratio */
  {
    int xres, yres;
    
    mi_texture_info(paras->si_default.texture, &xres, &yres, NULL);

    tex_point.x = (tex_point.x -0.5) * (miScalar) yres / (miScalar) xres + 0.5;
  }


  if ((h)  &&
      (tex_point.x >= 0) && (tex_point.x <= 1) &&
      (tex_point.y >= 0) && (tex_point.y <= 1))
    {
      return mi_lookup_color_texture(result, state, 
				     paras->si_default.texture, &tex_point);
    }
  else
    return miFALSE;

}

