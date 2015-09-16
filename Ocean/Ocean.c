
/* Copyright (c) 1997 Lume Inc. */

#include <stdio.h>
#include <math.h>
#include <shader.h>
#include <mi_softshade.h>
#include <calc.h>
#include <lic.h>

#define NUM_WAVE 50

#define NAME "Ocean"
#define PROD PROD_LUMEWATER

struct wave
{
  int active;          /* is this wave "on" */

  double cosa, sina;   /* rotation transformation */
  double wavelength;
  double wavewidth;    /* heh... */
  double amplitude;
  double speed;
  double offset;

  int    fade;         /* does this wave "fade" for looping */
  int    fade_frame;   /* frame number when this wave "fades" */
  int    frames;

};



struct Ocean_param
{
  miScalar  animation_speed;
  miBoolean animation_loop_on;
  miInteger animation_loop_frames;

  miBoolean flats_on;
  miScalar  flats_size;
  miScalar  flats_variation;

  miBoolean position_object_on;
  miBoolean position_world_on;

  miBoolean position_directionless_on;
  miBoolean position_directed_on;
  miScalar  position_directed_angle;

  miBoolean shape_bump_on;
  miBoolean shape_displacement_on;

  miScalar  size_largest;
  miScalar  size_smallest;
  miInteger size_number;
  miScalar  size_steepness;
  
  struct wave *wave;
};




/* alloc and init the wave table */
struct wave *init_waves()
{
  struct wave *waves;
  int i;

  waves = mi_mem_allocate(sizeof(struct wave) * NUM_WAVE);
  if (!waves)
    mi_fatal("[Ocean] Cannot allocate wave struct");
  
  for (i = 0; i < NUM_WAVE; i++)
    waves[i].active = 0;

  return waves;
}


/* print the wave table */
void dump_waves(struct wave *waves)
{
  int i;

  for (i = 0; i < NUM_WAVE; i++)
    {
      if (waves[i].active)
	mi_info("%d: wl:%f a:%f f:%d\n",i,
		waves[i].wavelength,
		waves[i].amplitude,
		waves[i].fade_frame);
    }


}



/* add a wave to the wave table */
int new_wave(struct wave *waves,
	      double angle,
	      double wavelength, double wavewidth,
	      double amplitude,
	      double speed,
	      double offset,
	      int fade, int fade_frame, int frames)
{
  int i;

  for (i = 0; i < NUM_WAVE; i++)
    {
      if (!waves[i].active)
	{
	  waves[i].active = 1;

	  waves[i].sina = sin(angle);
	  waves[i].cosa = cos(angle);

	  waves[i].wavelength = wavelength;
	  waves[i].wavewidth  = wavewidth;
	  waves[i].amplitude  = amplitude;
	  waves[i].speed      = speed;
	  waves[i].offset     = offset;

	  waves[i].fade       = fade;
	  waves[i].fade_frame = fade_frame;
	  waves[i].frames     = frames;

	  return 1;
	}
    }

  return 0;
}




DLLEXPORT void Ocean_init(miState *state, struct Ocean_param *paras,
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
      paras->wave = init_waves();

      {
	double size_ratio;
	int size;

	if (paras->size_number < 1)
	  mi_fatal("[Ocean]  Must have one or more waves.");

	if (paras->size_number == 1)
	  size_ratio = 1.0;
	else
	  size_ratio = pow(paras->size_smallest / paras->size_largest, 
			   1.0/(paras->size_number - 1));


#define NUM_CLUSTER 3
#define NUM_FADE 2
#define K_SPEED 0.037
#define K_AMP 0.09
#define K_H 1.1
#define K_WAVEWIDTH 1.5
#define K_SPREAD 2.0 * PI


	for (size = 0; size < paras->size_number; size++)
	  {
	    int i;
	    double ratio;
	    double wavelength;
	    double amplitude;
	    int num_cluster;
	    double iangle;
	    double wave_width;

	    ratio = pow(size_ratio,size);

	    wavelength = paras->size_largest * ratio;
	    amplitude  = paras->size_largest * 
	      pow(ratio, K_H) * K_AMP * paras->size_steepness;


	    if (paras->position_directionless_on)
	      {
		num_cluster = NUM_CLUSTER;
		iangle = 0;
		wave_width = K_WAVEWIDTH;
	      }
	    if (paras->position_directed_on)
	      {
		num_cluster = 1;
		iangle = paras->position_directed_angle * PI / 180.0;
		wave_width = K_WAVEWIDTH;
	      }


	    for (i = 0; i < num_cluster; i++)
	      {
		int fade;
		int num_fade;
		double angle;


		angle = iangle + 
		  (i - (num_cluster-1)/2.0) * K_SPREAD / num_cluster;

		if (paras->animation_loop_on)
		  num_fade = NUM_FADE;
		else
		  num_fade = 1;

		for (fade = 0; fade < num_fade; fade++)
		  {
		    new_wave(paras->wave, angle, 
			     wavelength, wavelength * wave_width,
			     amplitude,
			     sqrt(wavelength) * paras->animation_speed 
			     * K_SPEED,
			     wavelength * angle * 100.0 + i*7 +fade*5,
			     paras->animation_loop_on,
			     i * (paras->animation_loop_frames/num_cluster) +
			     fade * (paras->animation_loop_frames/num_fade),
			     paras->animation_loop_frames);
		  }

	      }

	  }

      }
    }

}




DLLEXPORT void Ocean_exit(struct Ocean_param *paras)
{
  if (!paras)
    {
      /* global cleanup */
      lic_end(PROD);
    }

  else
    {
      mi_mem_release(paras->wave);
    }

}


double flats(miVector point, miScalar size, miScalar variation)
{
  miVector scaled_point;
  double amount;
  double r;

  scaled_point = point;

  mi_vector_div(&scaled_point, size);

  scaled_point.y = 3.0;  /* choose a good y so that we don't get
			    a flat at the origin */

  amount =  gain(mi_noise_3d(&scaled_point),0.95);

  r =  1.0 + (amount -0.5) * variation * 2.0;

  return r;
}



double fade(int frame, int frames)
{
  return (1.0 - cos(2.0 * PI * frame / frames)) / 2.0;
}



int frame_loop(int frame, int frames)
{
  int looped_frame;

  looped_frame = frame % frames;
  if (looped_frame < 0)
    looped_frame += frames;

  return looped_frame;
}




/* wave_height()
   Given a wave description and a surface point, return the
   wave's height at the specified point */


double wave_height(struct wave *wave, miVector point, int frame)
{
  double x,z;
  double height;

  /* Rotate coordinates to wave's direction */
  x = wave->cosa * point.x - wave->sina * point.z;
  z = wave->cosa * point.z + wave->sina * point.x;

  if (wave->fade)
    frame = frame_loop(frame - wave->fade_frame, wave->frames);
  


  {
    miVector noise_point;

    noise_point.x = x / wave->wavelength + wave->offset + 
      wave->speed * frame;
    noise_point.z = z / wave->wavewidth;
    noise_point.y = 0;

    height = (mi_noise_3d(&noise_point) -0.5) * 2.0 * wave->amplitude;
  }

  if (wave->fade)
    height *= fade(frame, wave->frames);

  return height;
}




/* wave_grad()
   Given a wave description and a surface point, return the
   wave's surface gradient at the specified point */

miVector wave_grad(struct wave *wave, miVector point, int frame)
{
  miVector rotated_grad;
  double x,z;

  if (wave->fade)
    frame = frame_loop(frame - wave->fade_frame, wave->frames);


  /* Rotate coordinates to wave's direction */
  x = wave->cosa * point.x - wave->sina * point.z;
  z = wave->cosa * point.z + wave->sina * point.x + point.y;
  
  {
    miVector noise_point;

    noise_point.x = x / wave->wavelength + wave->offset + 
      wave->speed * frame;
    noise_point.z = z / wave->wavewidth;
    noise_point.y = 0;

    mi_noise_3d_grad(&noise_point, &rotated_grad);

    rotated_grad.x *= wave->amplitude / wave->wavelength;
    rotated_grad.z *= wave->amplitude / wave->wavewidth;
    rotated_grad.y = 0;
    
    if (wave->fade)
      {
	double f;

	f = fade(frame, wave->frames);
	rotated_grad.x *= f;
	rotated_grad.z *= f;
      }



  }


  /* Rotate grad back */
  {
    miVector grad;
    
    grad.y = rotated_grad.y;
    grad.x = rotated_grad.x * wave->cosa + rotated_grad.z * wave->sina;
    grad.z = rotated_grad.z * wave->cosa - rotated_grad.x * wave->sina;

    return grad;
  }
}




DLLEXPORT miBoolean Ocean(miColor *result, miState *state,
		    struct Ocean_param *paras)
{
  int i;
  miVector point, new_normal;

  miVector bump;
  double height;


  if (paras->position_world_on)
    mi_point_to_world(state, &point, &state->point);
  else
    mi_point_to_object(state, &point, &state->point);


  if (paras->shape_bump_on)    /* compute bump */
    {
      bump.x = bump.y = bump.z = 0;

      for (i = 0; i < NUM_WAVE; i++)
	{
	  miVector tmp_grad;

	  if (paras->wave[i].active)
	    {
	      tmp_grad = wave_grad(&paras->wave[i], 
				   point, state->camera->frame);
      
	      mi_vector_add(&bump, &bump, &tmp_grad);
	    }
	}

      

      if (paras->flats_on)
	{
	  double f;

	  f = flats(point, paras->flats_size, paras->flats_variation);
	  bump.x *= f;
	  bump.z *= f;
	}

      if (paras->position_world_on)
	mi_vector_to_world(state, &new_normal, &state->normal);
      else
	mi_vector_to_object (state, &new_normal, &state->normal);

      mi_vector_add(&new_normal, &new_normal, &bump);

      if (paras->position_world_on)
	mi_vector_from_world(state, &state->normal, &new_normal);
      else
	mi_vector_from_object(state, &state->normal, &new_normal);
	


      mi_vector_normalize(&state->normal);
      state->dot_nd = mi_vector_dot(&state->normal, &state->dir);

      result-> r = result->g = result->b = 0;
    }




  if (!paras->shape_bump_on)  /* compute displacement */
    {
      height = 0;
      
      for (i = 0; i < NUM_WAVE; i++)
	{
	  if (paras->wave[i].active)
	    height += wave_height(&paras->wave[i], 
				  point, state->camera->frame);
	}
      if (paras->flats_on)
	height *= flats(point, paras->flats_size, paras->flats_variation);

      result->r = result->g = result->b = height;

    }


  return miTRUE;
}
