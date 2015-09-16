
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>
#include <lume_state.h>

#define NAME "Wet"
#define PROD PROD_LUMEWATER

struct Wet_param
{
  struct soft_color si_default;

  miScalar dum;
};




DLLEXPORT void Wet_init(miState *state, struct Wet_param *paras,
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




DLLEXPORT void Wet_exit(struct Wet_param *paras)
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



DLLEXPORT miBoolean Wet(miColor *result, miState *state,
		    struct Wet_param *paras)
{
  double influence;

  soft_color(result, state, &paras->si_default);

  if ( (state->parent) &&
       (is_stained_lume_state(state->parent)) )
    result->a = 1;
  else
    result->a = 0;

  return miTRUE;
}


