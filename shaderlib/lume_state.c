
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <shader.h>
#include "lume_state.h"


miBoolean init_lume_state(miState *state)
{
  struct lume_state *i;

  i = mi_mem_allocate(sizeof (struct lume_state));
  if (!i)
    return miFALSE;

  /* set default behavior */
  i->cookie       = LUME_COOKIE;
  i->did_multiray = miFALSE;
  i->stained      = miFALSE;

  state->user = i;
  state->user_size = sizeof (struct lume_state);

  return miTRUE;
}


miBoolean is_lume_state(miState *state)
{
  return (miBoolean)((state->user_size == sizeof (struct lume_state)) &&
		     (((struct lume_state *)state->user)->cookie == LUME_COOKIE));
}



void free_lume_state(miState *state)
{
  if (is_lume_state(state))
    {
      mi_mem_release(state->user);
      state->user = NULL;
      state->user_size = 0;
      return;
    }
  else
    mi_warning("Bad state.\n");
}


miBoolean lume_did_multiray(miState *state)
{
  miState *s;

  for (s = state; s; s=s->parent)
    {
      
      if ( (is_lume_state(s)) &&
	   (((struct lume_state *)s->user)->did_multiray) )
	return miTRUE;
    }

  return miFALSE;
}
	  
	  
  
miBoolean is_stained_lume_state(miState *state)
{
  return (miBoolean)(is_lume_state(state) &&
		     (((struct lume_state *)state->user)->stained));
}


void lume_print_state(char *str, miState *state)
{
  char *type, *multiray, *stained;

  type = multiray = stained = "";

  switch(state->type)
    {
    case miRAY_EYE:
      type = "eye";
      break;
    case miRAY_TRANSPARENT:
      type = "transparent";
      break;
    case miRAY_REFLECT:
      type = "reflect";
      break;
    case miRAY_REFRACT:
      type = "refract";
      break;
    case miRAY_LIGHT:
      type = "light";
      break;
    case miRAY_SHADOW:
      type = "shadow";
      break;
    case miRAY_ENVIRONMENT:
      type = "environment";
      break;
    case miRAY_NONE:
      type = "NONE";
      break;
    case miRAY_NO_TYPES:
      type = "NO_TYPES";
      break;
    default:
      type = "UNKNOWN";
      break;
    }

  if (is_lume_state(state))
      {
	if (((struct lume_state *)state->user)->did_multiray)
	  multiray = " (multiray)";

	if (((struct lume_state *)state->user)->stained)
	  stained = " (stained)";
      }

  mi_info("%8s: %8s (%x)%s%s\n",str,type,state,multiray,stained);
}


void lume_dump_state(miState *state)
{
  miState *s;

  lume_print_state("Current", state);
  for (s = state->parent; s ; s = s->parent)
    lume_print_state("Parent", s);
}
