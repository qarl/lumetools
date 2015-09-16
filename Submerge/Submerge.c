
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <lic.h>

#define NAME "Submerge"
#define PROD PROD_LUMEWATER

#define F 0.98

struct Submerge_param
{
  miColor  water_color;
  miScalar height;
  miScalar gradation;
  miScalar opacity;
};




DLLEXPORT void Submerge_init(miState *state, struct Submerge_param *paras,
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




DLLEXPORT void Submerge_exit(struct Submerge_param *paras)
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


/* lume_underwater - do underwater shading.  For derivation, see
   "Tale of a Shader Company, Feb 28" */

void lume_underwater(miColor *result, miColor *kc,
		     miScalar kd, miScalar ks,
		     miScalar e0, miScalar es,
		     miScalar l)
{
  double t1, t2, t3, L, D;

  /* temp value, reused in comp */
  t1 = -kd * pow(F, ks * e0);
  t2 = kd + ks * es;
  t3 = pow(F, t2 * l) - 1;

  L = t1 / t2 * t3;

  D = pow(F, l*kd);

  result->r = (result->r * D) + (kc->r * L);
  result->g = (result->g * D) + (kc->g * L);
  result->b = (result->b * D) + (kc->b * L);

}

/* lume_clipped_underwater - underwater shading, but clip rays to
   under the surface plane */

void lume_clipped_underwater(miColor *result, miColor *kc,
			     miScalar kd, miScalar ks,
			     miScalar e0, miScalar es,
			     miScalar l)
{
  miScalar e1;

  if (l == 0)  
    l = LUME_INF;

  e1 = e0 + es * l;

  if ((e0 < 0) && (e1 < 0))
    return;

  if ((e0 >= 0) && (e1 >= 0))
    {
      lume_underwater(result, kc, kd, ks, e0, es, l);
      return;
    }
  
  if ((e0 < 0) && (e1 >= 0))
    {
      l = e1 / es;
      e0 = 0;

      lume_underwater(result, kc, kd, ks, e0, es, l);
      return;
    }

  if ((e0 >= 0) && (e1 < 0))
    {
      l = -e0/es;
      e1 = 0;

      lume_underwater(result, kc, kd, ks, e0, es, l);
      return;
    }


  /* not reached */
}




DLLEXPORT miBoolean Submerge(miColor *result, miState *state,
		    struct Submerge_param *paras)
{
  miVector org;
  miVector dir;

  if (state->type >=miRAY_LIGHT)
    return miTRUE;

  mi_point_to_world(state, &org, &state->org);
  mi_vector_to_world(state, &dir, &state->dir);

  lume_clipped_underwater(result, &paras->water_color,
			  paras->opacity, paras->gradation,
			  paras->height - org.y, -dir.y,
			  state->dist);

  return miTRUE;
}
