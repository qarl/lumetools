
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>
#include <lume_state.h>

#define NAME "Stain"
#define PROD PROD_LUMELANDSCAPE

struct Stain_param
{
  struct soft_material si_default;

  miBoolean dum;
};




DLLEXPORT void Stain_init(miState *state, struct Stain_param *paras,
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




DLLEXPORT void Stain_exit(struct Stain_param *paras)
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




DLLEXPORT miBoolean Stain(miColor *result, miState *state,
		    struct Stain_param *paras)
{
  miBoolean ret;

  if (!init_lume_state(state))
    mi_fatal("Cannot allocate lume_state\n");
    
  ((struct lume_state *)state->user)->stained = miTRUE;

  
  ret = soft_material(result, state, &paras->si_default);

 
  free_lume_state(state);
  
  return ret;

}

