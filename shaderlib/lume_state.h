#define LUME_COOKIE  0xb00fa


struct lume_state
{
  int cookie;
  /* code to ident this as a lume_state */

  miBoolean  did_multiray;
  /* this shader spawned a bunch of rays (radiosity/brushed metal) */

  miBoolean  stained;
  /* subsequent rays should be considered "stained" */
};


miBoolean init_lume_state(miState *state);
miBoolean is_lume_state(miState *state);
void free_lume_state(miState *state);
miBoolean lume_did_multiray(miState *state);
miBoolean is_stained_lume_state(miState *state);
void lume_dump_state(miState *state);
void lume_print_state(char *str, miState *state);
     
