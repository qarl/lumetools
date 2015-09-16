
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <lic.h>

#define NAME "Glow"
#define PROD PROD_LUMELIGHT

struct Glow_param
{
  struct soft_material si_default;

  miScalar brightness;

  miBoolean color_texture_on;
  miInteger color_texture_number;

  miBoolean color_solid_on;
  miColor   color_solid_color;

  miBoolean mix_over_on;
  miBoolean mix_under_on;

};




DLLEXPORT void Glow_init(miState *state, struct Glow_param *paras,
		    miBoolean *req)
{
  if (req)
    {
      /* global shader init here, paras == NULL */

      *req = miTRUE;

      lic(state, NAME, PROD);

    }

  else
    {
      if (state->options->preview_mode)
	{
	  paras->color_solid_on = miTRUE;
	  paras->color_texture_on = miFALSE;
	}

      /* instance init here, paras valid */

      if ((paras->color_texture_on) &&
	  ((paras->color_texture_number > paras->si_default.n_texture) ||
	   (paras->color_texture_number <= 0)))
	mi_fatal("Glow texture #%d does not exist on this object.\n",
		 paras->color_texture_number);

    }
}




DLLEXPORT void Glow_exit(struct Glow_param *paras)
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



DLLEXPORT miBoolean Glow(miColor *result, miState *state,
		    struct Glow_param *paras)
{
  miColor glow;
  struct soft_material m;
  miScalar ior_in, ior_out;


  if (state->type == miRAY_SHADOW)
    {
	    if (!mi_mtl_is_casting_shadow(state, &paras->si_default))
	      return miFALSE;
    }
  else
    mi_mtl_refraction_index(state, &paras->si_default, &ior_in, &ior_out);


  m = paras->si_default;
  
  if (paras->color_texture_on)
    {
      struct soft_material tmp_mat;
      struct Tex *tmp_tex;
      int tex_size;

      tmp_mat = paras->si_default;

      tex_size = sizeof(struct Tex) * tmp_mat.n_texture;
      tmp_tex = mi_mem_allocate(tex_size);

      tmp_mat.texture = tmp_tex;
      tmp_mat.i_texture = 0;



      /* get a sample of the glow texture */
      {
	int i;

	/* copy textures */
	memcpy(tmp_tex,
	       paras->si_default.texture + 
	       sizeof(struct Tex)*paras->si_default.i_texture,
	       tex_size);

	/* turn off blend on all other textures */
	for (i = 0; i < tmp_mat.n_texture; i++)
	  if (i != paras->color_texture_number - 1)
	    tmp_mat.texture[i + tmp_mat.i_texture].blend = 0;

	/* sample */
	{
	  miVector dumv;
	  miScalar dums;
	  
	  m.diffuse.r = m.diffuse.g = m.diffuse.b = 0;
	  mi_mtl_textures(state, &m, &tmp_mat, &dumv, &dums);
	  
	  glow = m.diffuse;
	}
      }

      { /* sample other textures */

	/* copy textures */
	memcpy(tmp_tex,
	       paras->si_default.texture + 
	       sizeof(struct Tex)*paras->si_default.i_texture,
	       tex_size);

	/* turn off glow texture */

	tmp_mat.texture[tmp_mat.i_texture + 
		       paras->color_texture_number - 1].blend = 0;

	/* sample */
	m = paras->si_default;
	mi_mtl_textures(state, &m, &tmp_mat, 
			&state->normal, &state->dot_nd);
      }

      mi_mem_release(tmp_tex);
    }

  else /* solid glow */
    {
      glow = paras->color_solid_color;

      mi_mtl_textures(state, &m, &paras->si_default,
		      &state->normal, &state->dot_nd);
    }

  if (paras->mix_under_on)
    {
      glow.r *= m.diffuse.r;
      glow.g *= m.diffuse.g;
      glow.b *= m.diffuse.b;
    }


  if (state->type == miRAY_SHADOW)
    return mi_mtl_compute_shadow(result, &m);

  mi_mtl_illumination(result, state, &m, &paras->si_default,
		      ior_in, ior_out);

  mi_mtl_reflection(result, state, &m);

  mi_mtl_refraction(result, state, &m, ior_in, ior_out);
  
  
  /* Add glow */
    {
      result->r += glow.r * paras->brightness * (1.0 - m.transp);
      result->g += glow.g * paras->brightness * (1.0 - m.transp);
      result->b += glow.b * paras->brightness * (1.0 - m.transp);
    }

  return miTRUE;
		      
}
