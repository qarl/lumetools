
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <gamma.h>
#include <color.h>
#include <lic.h>


#define NAME "Adjustments"
#define PROD PROD_LUMEWORKBENCH

struct Adjustments_param
{
  miScalar  brightness;
  miScalar  contrast;

  miScalar  hue;
  miScalar  saturation;
  miScalar  lightness;

  miInteger input_high;
  miInteger input_low;
  miScalar  input_shift;
  miInteger output_high;
  miInteger output_low;

  miBoolean region_entire_on;
  miBoolean region_specific_on;

  int       i_region_specific_objects;
  int       n_region_specific_objects;
  miUint    *region_specific_objects;
  
};

typedef unsigned char channel256;

#define IMAGE256MAX 255

struct image256_cell
{
  channel256 r;
  channel256 g;
  channel256 b;
  channel256 a;
  channel256 on;
};


struct image256
{
  int xres;
  int yres;
  struct image256_cell *cells;
  struct image256_cell *cells_end;
};


DLLEXPORT void Adjustments_init(miState *state, struct Adjustments_param *paras,
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




DLLEXPORT void Adjustments_exit(struct Adjustments_param *paras)
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




/* image256_channel_remap -- efficient channel remapping */

void image256_channel_remap(struct image256 *image, 
			    channel256 *red_map,
			    channel256 *green_map,
			    channel256 *blue_map,
			    channel256 *alpha_map,
			    int mask)
{
  int x, y;
  struct image256_cell *cell;

  /* loop over cells */
  for (cell = image->cells; cell < image->cells_end; cell++)
    {
      if ((!mask) || (cell->on))
	{
	  if (red_map)
	    cell->r = red_map[cell->r];
	  if (green_map)
	    cell->g = green_map[cell->g];
	  if (blue_map)
	    cell->b = blue_map[cell->b];
	  if (alpha_map)
	    cell->a = alpha_map[cell->a];
	}
    }

}
		     

struct image256 *image256_load(miImg_image *frame_rgba,
			       miImg_image *frame_label,
			       int xres, int yres,
			       int num_tags, miUint *tags)
{
  struct image256 *image;
  struct image256_cell *cells;
  int x,y;
  miColor color;
  miUint label;

  image = mi_mem_allocate(sizeof(struct image256));
  cells = mi_mem_allocate(sizeof(struct image256_cell) * xres * yres);

  image->xres  =  xres;
  image->yres  =  yres;
  image->cells = cells;
  image->cells_end = cells + xres * yres;

  for (x=0; x<xres; x++)
    for (y=0; y<yres; y++)
      {
	struct image256_cell *cell;

	mi_img_get_color(frame_rgba, &color, x, y);
	mi_img_get_label(frame_label, &label, x, y);

	cell = &cells[x + y*xres];

	cell->r  = color.r * IMAGE256MAX;
	cell->g  = color.g * IMAGE256MAX;
	cell->b  = color.b * IMAGE256MAX;
	cell->a  = color.a * IMAGE256MAX;

	{
	  int i;

	  cell->on = 0;

	  for (i = 0; i < num_tags; i++)
	    if (label == tags[i])
	      cell->on = 1;

	}

      }

  return image;
}	



void image256_save(struct image256 *image,
		   miImg_image *frame_rgba)
{
  int x,y;
  miColor color;

  for (x=0; x < image->xres; x++)
    for (y=0; y < image->yres; y++)
      {
	color.r = (miScalar)image->cells[x + y*image->xres].r / IMAGE256MAX;
	color.g = (miScalar)image->cells[x + y*image->xres].g / IMAGE256MAX;
	color.b = (miScalar)image->cells[x + y*image->xres].b / IMAGE256MAX;
	color.a = (miScalar)image->cells[x + y*image->xres].a / IMAGE256MAX;

	mi_img_put_color(frame_rgba, &color, x, y);
      }

}



void image256_destroy(struct image256 *image)
{

  mi_mem_release(image->cells);
  mi_mem_release(image);

}


/************************************************************************
  here begins the reverse engineered photoshop color adjustments
  **********************************************************************/


void filter_hue(struct image256 *image, miScalar hue, int mask)
{
  struct image256_cell *cell;
    
  if (hue == 0)
    return;

  for (cell = image->cells; cell < image->cells_end; cell++)
    {
      if ((!mask) || (cell->on))
	{
	  miScalar h,s,v;
	  miScalar r,g,b;

	  r = (miScalar) cell->r / IMAGE256MAX;
	  g = (miScalar) cell->g / IMAGE256MAX;
	  b = (miScalar) cell->b / IMAGE256MAX;

	  convert_rgb_hsv(r, g, b, &h, &s, &v);

	  h += hue;

	  convert_hsv_rgb(h, s, v, &r, &g, &b);

	  cell->r = r * IMAGE256MAX;
	  cell->g = g * IMAGE256MAX;
	  cell->b = b * IMAGE256MAX;
	}
    }

}




/* this is pure lunacy.  I fucking hate photoshop. */
void mmm(channel256 *a, channel256 *b, channel256 *c,
	 channel256 **min, channel256 **mid, channel256 **max)
{

  if (*a > *b)
    {
      *max = a;
      *min = b;
    }
  else
    {
      *max = b;
      *min = a;
    }


  if (*c > **max)
    {
      *mid = *max;
      *max = c;
      return;
    }

  if (*c < **min)
    {
      *mid = *min;
      *min = c;
      return;
    }

  *mid = c;

}




/* dear lord, this is disgusting.  please forgive me for giving
   it new life */
void filter_saturation_positive(struct image256 *image, 
				miScalar saturation, int mask)
{
  struct image256_cell *cell;
  float s;
  channel256 *min, *mid, *max;

  s = saturation / 100.0;

  if (s == 1)
    s = 0.9999;


  /* loop over cells */
  for (cell = image->cells; cell < image->cells_end; cell++)
    {
      if ((!mask) || (cell->on))
	{
	  channel256 oldmin, oldmax;
	  int newmin, newmax;
	  channel256 D;

	  mmm(&cell->r, &cell->g, &cell->b, &min, &mid, &max);


	  D = *max - *min;
	  oldmin = *min;
	  oldmax = *max;


	  /* again I say, this is lunacy.  this is photoshop */
	  newmin = *min - D * s / (1 - s) / 2;
	  newmax = *max + D * s / (1 - s) / 2;
	  
	  if (newmin < 0)
	    {
	      newmin = 0;
	      newmax = *max + *min;
	    }

	  if (newmax > IMAGE256MAX)
	    {
	      newmax = IMAGE256MAX;
	      newmin = *min - (IMAGE256MAX - *max);
	    }


	  *min = newmin;
	  *max = newmax;

	  *mid = range_scale(*mid, oldmin, oldmax, *min, *max);

	}

    }
}



void filter_saturation_negative(struct image256 *image, 
				miScalar saturation, int mask)
{
  struct image256_cell *cell;
  float s;

  s = saturation / -100.0;

  /* loop over cells */
  for (cell = image->cells; cell < image->cells_end; cell++)
    {
      int mid;
      
      if ((!mask) || (cell->on))
	{
	  mid = (max3(cell->r, cell->g, cell->b) + 
		 min3(cell->r, cell->g, cell->b)) / 2;

	  cell->r = cell->r * (1.0 - s) + mid * s;
	  cell->g = cell->g * (1.0 - s) + mid * s;
	  cell->b = cell->b * (1.0 - s) + mid * s;

	}

    }
}





void filter_saturation(struct image256 *image, miScalar saturation, int mask)
{
  channel256 map[256];
  int i;
  double l;

  if (saturation > 0)
    filter_saturation_positive(image, saturation, mask);
  
  if (saturation < 0)
    filter_saturation_negative(image, saturation, mask);
  
}


void filter_lightness(struct image256 *image, miScalar lightness, int mask)
{
  channel256 map[256];
  int i;
  double l;

  if (lightness == 0)
    return;

  l = lightness / 100.0;

  for(i = 0; i < 256; i++)
    {
      int new;

      if (l > 0)
	new = i * (1 - l) + IMAGE256MAX * l;

      
      if (l < 0)
	new = i * (1 + l);


      if (new > IMAGE256MAX)
	new = IMAGE256MAX;
      if (new < 0)
	new = 0;

      map[i] = new;
    }
  
  image256_channel_remap(image, map, map, map, NULL, mask);

}




void filter_brightness(struct image256 *image, miScalar brightness, int mask)
{
  channel256 map[256];
  int new;
  int i;

  if (brightness == 0)
    return;

  for(i = 0; i < 256; i++)
    {
      new = i + brightness;
      if (new > IMAGE256MAX)
	new = IMAGE256MAX;
      if (new < 0)
	new = 0;

      map[i] = new;
    }
  
  image256_channel_remap(image, map, map, map, NULL, mask);

}



void filter_contrast(struct image256 *image, miScalar contrast, int mask)
{
  channel256 map[256];
  int new;
  int i;
  int mean;
  double c;

  {
    int total;
    int count;
    struct image256_cell *cell;
    struct image256_cell *endcell;


    if (contrast == 0)
      return;
    

    total = count = 0;

    for (cell = image->cells; cell < image->cells_end; cell++)
      if ((!mask) || (cell->on))
	{  
	  total += max3(cell->r, cell->g, cell->b);
	  count++;
	}

    mean = total / count;
  }

  c = contrast / 100.0;

  for(i = 0; i < 256; i++)
    {
      if (i == mean)
	map[i] = i;

      if ( (i < mean) && (c < 0) )
	map[i] = range_scale(i, 0, mean, -c * mean, mean);
      if ( (i > mean) && (c < 0))
	map[i] = range_scale(i, mean, IMAGE256MAX, mean, 
			     IMAGE256MAX + c * (IMAGE256MAX -mean));
      if ( (i < mean) && (c > 0) )
	map[i] = range_scale(i, c * mean , mean, 0, mean);
      if ( (i > mean) && (c > 0) )
	map[i] = range_scale(i, mean , IMAGE256MAX - c * (IMAGE256MAX - mean),
			    mean, IMAGE256MAX);
    }
  

  image256_channel_remap(image, map, map, map, NULL, mask);

}



void filter_levels(struct image256 *image, 
		   miScalar input_low, miScalar input_shift, 
		   miScalar input_high,
		   miScalar output_low, miScalar output_high,
		   int mask)
{
  channel256 map[256];
  int i;

  if ((input_low == 0) && 
      (input_high == 255) &&
      (input_shift == 1.0) &&
      (output_low == 0) &&
      (output_high == 255))
    return;


  for(i = 0; i < 256; i++)
    {
      double f;
      channel256 new;

      f = range_scale((float)i/IMAGE256MAX, 
		      (float)input_low/IMAGE256MAX,
		      (float)input_high/IMAGE256MAX,
		      0,1);

      f = pow(f, 1.0 / input_shift);

      f = range_scale(f, 0,1,
		      (float)output_low/IMAGE256MAX,
		      (float)output_high/IMAGE256MAX);

      {
	int i;
	i = (float)f * IMAGE256MAX;

	if (i > IMAGE256MAX)
	  i = IMAGE256MAX;
	if (i < 0)
	  i = 0;
	
	new = i;
      }

      map[i] = new;
    }
  
  image256_channel_remap(image, map, map, map, NULL, mask);

}





DLLEXPORT miBoolean Adjustments(miOutstate *state,
			  struct Adjustments_param *paras)
{
  struct image256 *image;

  lic(NULL, NAME, PROD);

  image = image256_load(state->frame_rgba, state->frame_label,
			state->xres, state->yres,
			paras->n_region_specific_objects,
			&paras->region_specific_objects
                               [paras->i_region_specific_objects]);



  /* filter it */
  filter_brightness(image, paras->brightness, paras->region_specific_on);

  filter_contrast(image, paras->contrast, paras->region_specific_on);

  filter_hue(image, paras->hue, paras->region_specific_on);

  filter_saturation(image, paras->saturation, paras->region_specific_on);

  filter_lightness(image, paras->lightness, paras->region_specific_on);

  filter_levels(image, 
		paras->input_low, paras->input_shift, paras->input_high,
		paras->output_low, paras->output_high,
		paras->region_specific_on);

  image256_save(image, state->frame_rgba);

  image256_destroy(image);

  lic_end(PROD);

  return miTRUE;
}
