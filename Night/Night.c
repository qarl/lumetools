
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <color.h>
#include <lic.h>

#define NAME "Night"
#define PROD PROD_LUMELIGHT

struct Night_param
{
  miScalar strength;
  miScalar limit;
};




DLLEXPORT void Night_init(miState *state, struct Night_param *paras,
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




DLLEXPORT void Night_exit(struct Night_param *paras)
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




DLLEXPORT miBoolean Night(miColor *result, miState *state,
		    struct Night_param *paras)
{
  double i;
  miBoolean hit;

  hit = mi_trace_eye(result, state, &state->org, &state->dir);

  i = intensity(*result) * paras->strength;

  i = soft_clamp(i, paras->limit);


  result->r += i;
  result->g += i;
  result->b += i;

  return hit;
}

