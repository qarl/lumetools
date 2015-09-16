
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "Distortion"
#define PROD PROD_LUMEWORKBENCH

#define K_DISTORT 0.25

struct Distortion_param
{
  miBoolean type_pincushion_on;
  miBoolean type_barrel_on;
  miScalar  amount;
};




DLLEXPORT void Distortion_init(miState *state, struct Distortion_param *paras,
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




DLLEXPORT void Distortion_exit(struct Distortion_param *paras)
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




DLLEXPORT miBoolean Distortion(miColor *result, miState *state,
		    struct Distortion_param *paras)
{
  miScalar h;
  miScalar d;
  miVector distorted_dir;
  miVector dir;

  mi_vector_to_camera(state, &dir, &state->dir);


  h = fabs(sqrt(dir.x * dir.x + dir.y * dir.y) /
	   dir.z);

  d = paras->amount * h * h * K_DISTORT;

  if (paras->type_pincushion_on)
    d = -d;

  distorted_dir.z = state->dir.z;

  distorted_dir.x = state->dir.x * (1 + d);

  distorted_dir.y = state->dir.y * (1 + d);

  mi_vector_normalize(&distorted_dir);


  return mi_trace_eye(result, state, &state->org, &distorted_dir);
		
}

