
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <gamma.h>
#include <lic.h>

#define NAME "Glare"
#define PROD PROD_LUMELIGHT

#define K_B      0.0006    /* brightness scale */
#define FALL_POW -2.3      /* Falloff power */

#define NAN_WARN "[Glare] invalid color (NaN) found at coordinates %d,%d\n"
#define INF_WARN "[Glare] invalid color (Inf) found at coordinates %d,%d\n"

struct Glare_param
{
  miScalar  spread;

  miBoolean verbose_on;
  miBoolean overlay_on;

  miBoolean quality_fastest_on;
  miBoolean quality_faster_on;
  miBoolean quality_average_on;
  miBoolean quality_better_on;
  miBoolean quality_best_on;

  miBoolean rays_on;
  miTag     rays_image;
  miScalar  rays_size;
  miScalar  rays_contrast;

  miBoolean region_entire_on;
  miBoolean region_specific_on;
  int       i_region_specific_objects;
  int       n_region_specific_objects;
  miUint    *region_specific_objects;
};


DLLEXPORT void Glare_init(miState *state, struct Glare_param *paras,
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




DLLEXPORT void Glare_exit(struct Glare_param *paras)
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



int glare_object(miOutstate *state, struct Glare_param *paras,
		  int x, int y)
{
  int i;
  miUint label;
  
  if (paras->region_entire_on)
    return 1;

  mi_img_get_label(state->frame_label, &label, x,y);

  for (i = 0; i < paras->n_region_specific_objects; i ++)
    if (label == 
	paras->region_specific_objects[paras->i_region_specific_objects + i])
      return 1;

  return 0;
}


DLLEXPORT miBoolean Glare(miOutstate *state,
		    struct Glare_param *paras)
{
  miColor *image_in;
  miColor *image_out;
  float *glare_box;
  int box_size;            /* size of the effect box */
  double min;              /* threshold for affecting distant pixels */
  double threshold;        /* limit at which a pixel contributes */
  double scaling;          /* make glare resolution independent */

  lic(NULL, NAME, PROD);


  if ((paras->rays_on) && (paras->rays_image == NULL))
    mi_fatal("[Glare] Streaks image invalid.\n");


  /* set quality */
  if (paras->quality_fastest_on)
    {
      min       = 0.00031;
      threshold = 3.0;
    }
  if (paras->quality_faster_on)
    {
      min       = 0.0001;
      threshold = 2.0;
    }
  if (paras->quality_average_on)
    {
      min       = 0.000031;
      threshold = 1.1;
    }
  if (paras->quality_better_on)
    {
      min       = 0.00001;
      threshold = 0.9;
    }
  if (paras->quality_best_on)
    {
      min       = 0.00001;
      threshold = 0.5;
    }

  /* Determine scaling for resolution independence */
  /* scaling = sqrt(state->xres * state->xres + state->yres * state->yres) / 
    720.0; */
  scaling = 1;


  /* allocate out frame buffers */

  if (paras->verbose_on)
    mi_info("[Glare] Allocating image buffers.\n");
  image_in  = mi_mem_allocate(sizeof(miColor) * state->xres * state->yres);
  image_out = mi_mem_allocate(sizeof(miColor) * state->xres * state->yres);


  { /* copy the image into our buffer,
     also, set the copy the alpha channel directly to the 
     outgoing image */

    int x,y;
    miColor color;
    for (x=0; x<state->xres; x++)
      for (y=0; y<state->yres; y++)
	{
	  mi_img_get_color(state->frame_rgba, &color, x,y);
	  image_in[x+y*state->xres] = color;
	  image_out[x+y*state->xres].a = color.a;
	}
  }

  { /* Determine the size of the glare-box */

    int x,y;
    float M,m;
    miColor color;

    M = threshold;

    for (x=0; x<state->xres; x++)
      for (y=0; y<state->yres; y++)
	{
	  color = image_in[x+y*state->xres];

	  m = max3(color.r, color.g, color.b);

	  if (m != m)  /* is NaN */
	    {
	      mi_warning(NAN_WARN,x,y);
	      m = 0;
	    }

	  if ((m + 1) == m) /* is Infinity */
	    {
	      mi_warning(INF_WARN,x,y);
	      m = 0;
	    }
	  


	  if (m > M)
	    {
	      if ((m > threshold) &&
		  (glare_object(state, paras, x,y)))
		{
		  M = m;


		  box_size = (int)(paras->spread * scaling *
				   pow(autogamma_inverse_scalar(min)
				       /m/K_B,1/FALL_POW));
		}

	    }

	}

    if (box_size > state->xres)
      box_size = state->xres;

    if (box_size > state->yres)
      box_size = state->yres;

    if (box_size == 0)
      {
	mi_info("[Glare] Nothing to glare.\n");
	mi_mem_release(image_in);
	mi_mem_release(image_out);

	return miTRUE;
      }

  }


  if (paras->verbose_on)
    mi_info("[Glare] Allocating glare buffer (%dx%d).\n", 
	    box_size, box_size);
  glare_box = mi_mem_allocate(sizeof(float) * box_size * box_size * 4);



  { /* compute the glare_box */
    int x,y;

    for (x = -box_size; x < box_size; x++)
      for (y = -box_size; y < box_size; y ++)
	{
	  double f,d;
	  miColor p;
		    
	  d = sqrt(x*x + y*y);

	  if (( 0 == x )&&( 0 == y))
	    {
	      if (paras->overlay_on)
		f = 0;
	      else
		f = 1;
	    }

	  
	  else
	    {
	      f = autogamma_scalar(K_B * pow(d/paras->spread/scaling,
					     FALL_POW));
	      if (paras->rays_on)
		{
		  miColor c;
		  miVector p;
		
		  p.x = 0.5 + (double) x / state->xres;
		  p.y = 0.5 + (double) y / state->yres;

		  mi_lookup_color_texture(&c, NULL, paras->rays_image,
					  &p);

		  f *= ((c.r + c.g + c.b)/3 * paras->rays_contrast +
			1.0 - paras->rays_contrast);
		}
	    }
	  glare_box[(x+box_size) + (y+box_size)*box_size*2] = f;
	}

  }

  


  { /* Do glare */
    int x,y;
    int maxb = 0;
    int verbose_count;
    miColor color;

    verbose_count = 10;

    for (x=0; x < state->xres; x++)
      for (y=0; y < state->yres; y++)
	{
	  int bx, by;
	  int b;
	  miScalar c;


	  if ((x%verbose_count == 0) && 
	      (y == 0) && (paras->verbose_on))
	    {
	      mi_info("[Glare]  %.1f%% complete.\n",
		      (float) x/state->xres * 100.0);
	    }
			    

	  
	  color = image_in[x+y*state->xres];

	  c = max3(color.r, color.g, color.b);

	  if (c <= threshold)
	    b = 0;
	  else if ((c != c) || (c == c +1))
	    b = 0;
	  else if (!glare_object(state, paras, x, y))
	    b = 0;
	  else
	    {
	      b = (int)(paras->spread * scaling *
			pow(autogamma_inverse_scalar(min)/c/K_B,1/FALL_POW));
	      if (b > box_size - 1)
		b = box_size -1;
	    }


	  
	  for (by = -b ; by < b+1; by++)
	    {
	      float *gp;
	      miColor *iop;

	      gp  = &glare_box[(-b+box_size) + (by+box_size)*box_size*2];
	      iop = &image_out[(x-b)+(y+by)*state->xres];

	  for (bx = -b ; bx < b+1; bx++, gp++, iop++)
		{
		  int px, py;
		  px = x + bx;
		  py = y + by;
		  
		  if ((px >= 0) &&
		      (px < state->xres) &&
		      (py >= 0 ) &&
		      (py < state->yres))
		    {
		      float f;
		    
		      f = *gp;

		      iop->r += f * color.r;
		      iop->g += f * color.g;
		      iop->b += f * color.b;
		    }
		}
	    }
	}
  }


  
  { /* copy the image out to mi */
    int x,y;
    miColor color;
    for (x=0; x<state->xres; x++)
      for (y=0; y<state->yres; y++)
	{
	  miColor color;
	  miScalar new_alpha;

	  color = image_out[x+y*state->xres];

	  new_alpha = max3(color.r, color.b, color.g);

	  if (new_alpha > color.a)
	    color.a = new_alpha;

	  mi_img_put_color(state->frame_rgba, &color, 
			   x,y);
	}
  }

  mi_mem_release(image_in);
  mi_mem_release(image_out);
  mi_mem_release(glare_box);

  lic_end(PROD);

  return miTRUE;
}
