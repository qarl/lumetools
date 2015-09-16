
/* Copyright (c) 1997 Lume Inc. */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>

#include <shader.h>

#include<lmclient.h>
#include<lm_code.h>
#include<lm_attr.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* mi changed the name...? */
#ifndef mi_par_nthread
#define mi_par_nthread get_no_threads
#endif

#include "lic.h"
#include "lic_int.h"

#ifdef _WIN32
#define sleep(n) {int loop;for(loop = 0; loop < n*1000000; loop++);}
#endif

 
LM_CODE(code, ENCRYPTION_SEED1, ENCRYPTION_SEED2, VENDOR_KEY1,
	VENDOR_KEY2, VENDOR_KEY3, VENDOR_KEY4, VENDOR_KEY5);

LM_HANDLE *lm_job;

int checkout_successful;


char *id_to_name(int name_id, char *name)
{
  char *namei;

  namei = name;

  switch (name_id)
    {
    case PROD_LUMETOOLS:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'T';
      *namei++ = 'O';
      *namei++ = 'O';
      *namei++ = 'L';
      *namei++ = 'S';
      *namei++ =  0;
      break;

    case PROD_LUMELIGHT:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'L';
      *namei++ = 'I';
      *namei++ = 'G';
      *namei++ = 'H';
      *namei++ = 'T';
      *namei++ =  0;
      break;

    case PROD_LUMELANDSCAPE:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'L';
      *namei++ = 'A';
      *namei++ = 'N';
      *namei++ = 'D';
      *namei++ = 'S';
      *namei++ = 'C';
      *namei++ = 'A';
      *namei++ = 'P';
      *namei++ = 'E';
      *namei++ =  0;
      break;

    case PROD_LUMEWATER:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'W';
      *namei++ = 'A';
      *namei++ = 'T';
      *namei++ = 'E';
      *namei++ = 'R';
      *namei++ =  0;
      break;

    case PROD_LUMEMATTER:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'M';
      *namei++ = 'A';
      *namei++ = 'T';
      *namei++ = 'T';
      *namei++ = 'E';
      *namei++ = 'R';
      *namei++ =  0;
      break;

    case PROD_LUMEWORKBENCH:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'W';
      *namei++ = 'O';
      *namei++ = 'R';
      *namei++ = 'K';
      *namei++ = 'B';
      *namei++ = 'E';
      *namei++ = 'N';
      *namei++ = 'C';
      *namei++ = 'H';
      *namei++ =  0;
      break;

    case PROD_LUMECUSTOM:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'C';
      *namei++ = 'U';
      *namei++ = 'S';
      *namei++ = 'T';
      *namei++ = 'O';
      *namei++ = 'M';
      *namei++ =  0;
      break;

    default:
      *namei++ = 'L';
      *namei++ = 'U';
      *namei++ = 'M';
      *namei++ = 'E';
      *namei++ = 'e';
      *namei++ = 'r';
      *namei++ = 'r';
      *namei++ = 'o';
      *namei++ = 'r';
      *namei++ =  0;
      break;
    }

  return name;
}




void lic_end(int prod_id)
{
  char prod_name[100];

  if (!checkout_successful)
    return;

  id_to_name(prod_id, prod_name);

  lc_checkin(lm_job, prod_name, 0);
  lc_free_job(lm_job);
}



void lic_fail(char *software, char *msg)
{ 
  char *segv;

  mi_fatal("[%s] %s\n",software, msg);

  exit(1);
  segv = 0;
  *segv = 0;
}


int lume_multi_checkout (LM_HANDLE_PTR job, LM_CHAR_PTR feature,
			 LM_CHAR_PTR  version, int nlic, int flag,
			 VENDORCODE_PTR key, int dup, int id)
{
  int error;

#define _CHECKOUT lc_checkout(job, feature, version, nlic, flag, key, dup)

  if (!_CHECKOUT)
    return 0;

  sleep(1);

  if (!_CHECKOUT)
    return 0;

  sleep(1);

  if (!_CHECKOUT)
    return 0;

  sleep(1+id%4);

  if (!_CHECKOUT)
    return 0;

  sleep(1+id);

  if (!_CHECKOUT)
    return 0;

  return 1;
}



int lic(miState *state, char *software, int prod_id)
{
  FILE *license;
  char *license_filename;
  int i;
  char prod_name[100];
  char error[1000];
  int threads;
  /* vars for mr NT hack */
  int sock_mi_opentype;
  int sz_int = sizeof(int);
  int optval;

  checkout_successful = 0;

  if (state == NULL)
    return 1;


  if (state)
    mi_lock(state->global_lock);

  id_to_name(prod_id, prod_name);

  /* Under NT, mr does funky stuff to our sockets.  We need to temporarily
     fix the damage to get our checkouts to work */


#ifdef _WIN32
  if (getsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
		 (char *)&sock_mi_opentype, &sz_int))
    lic_fail(software, "getsockopt failure");
  optval = sock_mi_opentype & ~(SO_SYNCHRONOUS_NONALERT
				| SO_SYNCHRONOUS_ALERT);
  if (setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
		 (char *)&optval, sizeof(int)))
    lic_fail(software,"setsockopt failure");
#endif


  /* initialize the licensing */
  if (lc_init(NULL, VENDOR_NAME, &code, &lm_job))
    {
      char error[1000];
      sprintf(error, "Could not init licensing (%s)",
	      lc_errstring(lm_job));
    
      lic_fail(software,error);
    }

  /* set the default license file */
  lc_set_attr(lm_job, LM_A_LICENSE_DEFAULT, (LM_A_VAL_TYPE) LICFILE);
  /* tell flex to retry its checkouts */
  lc_set_attr(lm_job, LM_A_RETRY_CHECKOUT, (LM_A_VAL_TYPE) 1);
  /* tell flex to hold licenses for 5 minutes after a client crash */
  lc_set_attr(lm_job, LM_A_TCP_TIMEOUT, (LM_A_VAL_TYPE) 5 * 60);
  /* turn off the heartbeat */
  lc_set_attr(lm_job, LM_A_CHECK_INTERVAL, (LM_A_VAL_TYPE) -1);


  {
    int id;

    if (state)
      id = miTHREAD(mi_par_localvpu());
    else
      id = 1;

    if (lume_multi_checkout(lm_job, prod_name, LICVERSION, 1, 
			    LM_CO_NOWAIT,
			    &code, LM_DUP_USER | LM_DUP_HOST,
			    id))
      {
	char error_str[1000];
	sprintf(error_str, "Could not aquire main %s license (%s)",
		prod_name,
		lc_errstring(lm_job));
    
	lic_fail(software,error_str);
      }
  }


  threads = mi_par_nthread() - 1;

  if (threads > 0)
    {
      strcat(prod_name, "_THREAD");

      if (lume_multi_checkout(lm_job, 
			      prod_name, LICVERSION, threads, 
			      LM_CO_NOWAIT, &code, LM_DUP_USER | LM_DUP_HOST,
			      miTHREAD(mi_par_localvpu())))
	{
	  char error[1000];
	  sprintf(error, "Could not aquire %d thread %s license (%s)",
		  threads,
		  prod_name,
		  lc_errstring(lm_job));
    
	  lic_fail(software,error);
	}

    }


  /* undo the mr NT hack */
#ifdef _WIN32
  if (setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
		 (char *)&sock_mi_opentype, sizeof(int)))
    lic_fail(software, "setsockopt failure");
#endif


  if (state)
    mi_unlock(state->global_lock);

  checkout_successful = 1;

  return 1;
}
