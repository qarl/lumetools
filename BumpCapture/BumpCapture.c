
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "BumpCapture"
#define PROD PROD_LUMEWORKBENCH

struct BumpCapture_param
{
  miBoolean channel_rgb_on;
  miBoolean channel_alpha_on;
};




DLLEXPORT void BumpCapture_init(miState *state, struct BumpCapture_param *paras,
		    miBoolean *req)
{
  if (req)
    {
      /* global shader init here, paras == NULL */

      *req = miTRUE;
    }

  else
    {
      /* instance init here, paras valid */
    }
}




DLLEXPORT void BumpCapture_exit(struct BumpCapture_param *paras)
{
  if (!paras)
    {
      /* global cleanup */
    }

  else
    {
      /* instance cleanup */
    }

}




DLLEXPORT miBoolean BumpCapture(miOutstate *state,
		    struct BumpCapture_param *paras)
{
  int x,y;
  miScalar minz, maxz;

  lic(NULL, NAME, PROD);

  maxz = 0;
  minz = 1.0e+20;

  for (x=0; x<state->xres; x++)
    for (y=0; y<state->yres; y++)
      {
	miScalar z;

	mi_img_get_depth(state->frame_z, &z, x, y);
	if (z != 0)
	  {
	    if (z < minz)
	      minz = z;
	    if (z > maxz)
	      maxz = z;
	  }
      }
  
  for (x=0; x < state->xres; x++)
    for (y=0; y < state->yres; y++)
      {
	miColor color;
	miScalar z;
	miScalar d;

	mi_img_get_color(state->frame_rgba, &color, x, y);
	mi_img_get_depth(state->frame_z, &z, x, y);
	
	if (z <= 0.0)
	    z = maxz;

	d = (z - minz) / (maxz - minz);

	if (paras->channel_alpha_on)
	  color.a = d;

	if (paras->channel_rgb_on)
	  color.r = color.g = color.b = d;


	mi_img_put_color(state->frame_rgba, &color, x, y);
      }

  lic_end(PROD);

  return miTRUE;
}
