
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "WrapAround"
#define PROD PROD_LUMEWORKBENCH

struct WrapAround_param
{
  miBoolean dum;
};




DLLEXPORT void WrapAround_init(miState *state, struct WrapAround_param *paras,
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




DLLEXPORT void WrapAround_exit(struct WrapAround_param *paras)
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




DLLEXPORT miBoolean WrapAround(miColor *result, miState *state,
		    struct WrapAround_param *paras)
{
  miScalar x,y;

  state->refraction_level -= 1;


  {
    miScalar t;
    miVector dir;
    miVector point;

    mi_vector_to_camera(state, &dir, &state->dir);

    t = -2.0 / state->camera->aperture;

    x = (dir.x/dir.z * state->camera->focal) * t;
    y = (dir.y/dir.z * state->camera->focal) * t * 
      state->camera->aspect;
  }


  {
    miVector newdir;
    double k;

    newdir.y = sin(y * PI / 2.0);

    k = sqrt(1.0 - newdir.y * newdir.y);

    newdir.x = cos(x * PI) * k;
    newdir.z = sin(x * PI) * k;
			    
    mi_vector_from_world(state, &newdir, &newdir);

    state->refraction_volume = state->volume;

    state->point = state->org;
    return mi_trace_refraction(result, state, &newdir);

    /* perhaps one day...
       return mi_trace_eye(result, state, &state->org, &newdir);
    */


  }
  
}
