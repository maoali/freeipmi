/*
pef-config.c: Platform Event Filtering utility.
Copyright (C) 2005-2007 FreeIPMI Core Team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#ifdef HAVE_ERROR_H
#include <error.h>
#endif
#include <argp.h>
#include <assert.h>

#include "argp-common.h"
#include "ipmi-common.h"
#include "pef-config.h"
#include "pef-config-argp.h"
#include "pef-config-checkout.h"
#include "pef-config-commit.h"
#include "pef-config-diff.h"
#include "pef-config-info.h"
#include "pef-config-sections.h"

#include "freeipmi-portability.h"

void
_pef_config_state_data_init(pef_config_state_data_t *state_data)
{
  assert (state_data);

  memset(state_data, '\0', sizeof(pef_config_state_data_t));
  state_data->prog_data = NULL;
  state_data->dev = NULL;

  state_data->lan_channel_number_initialized = 0;
  state_data->number_of_lan_alert_destinations_initialized = 0;
  state_data->number_of_alert_strings_initialized = 0;
  state_data->number_of_alert_policy_entries_initialized = 0;
  state_data->number_of_event_filters_initialized = 0;
}

static int 
_pef_config (void *arg)
{
  pef_config_state_data_t state_data;
  pef_config_prog_data_t *prog_data;
  ipmi_device_t dev = NULL;
  struct section *sections = NULL;
  int exit_code = -1;
  pef_err_t ret = 0;

  prog_data = (pef_config_prog_data_t *) arg;
  
  if (!(dev = ipmi_device_create()))
    {
      perror("ipmi_device_create");
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }
  
  if (prog_data->args->common.host != NULL)
    {
      if (ipmi_open_outofband (dev,
                               IPMI_DEVICE_LAN,
                               prog_data->args->common.host,
                               prog_data->args->common.username,
                               prog_data->args->common.password,
                               prog_data->args->common.authentication_type,
                               prog_data->args->common.privilege_level,
                               prog_data->args->common.session_timeout,
                               prog_data->args->common.retry_timeout,
                               prog_data->debug_flags) < 0)
        {
          fprintf(stderr,
                  "ipmi_open_outofband: %s\n",
                  ipmi_device_strerror(ipmi_device_errnum(dev)));
          exit_code = EXIT_FAILURE;
          goto cleanup;
        }
    }
  else
    {
      if (!ipmi_is_root())
        {
          fprintf(stderr,
                  "%s: %s\n",
                  prog_data->progname,
                  ipmi_device_strerror(IPMI_ERR_PERMISSION));
          exit_code = EXIT_FAILURE;
          goto cleanup;
        }
      
      if (prog_data->args->common.driver_type == IPMI_DEVICE_UNKNOWN)
        {
          if (ipmi_open_inband (dev,
                                IPMI_DEVICE_OPENIPMI,
                                prog_data->args->common.disable_auto_probe,
                                prog_data->args->common.driver_address,
                                prog_data->args->common.register_spacing,
                                prog_data->args->common.driver_device,
                                prog_data->debug_flags) < 0)
            {
              if (ipmi_open_inband (dev,
                                    IPMI_DEVICE_KCS,
                                    prog_data->args->common.disable_auto_probe,
                                    prog_data->args->common.driver_address,
                                    prog_data->args->common.register_spacing,
                                    prog_data->args->common.driver_device,
                                    prog_data->debug_flags) < 0)
                {
                  if (ipmi_open_inband (dev,
                                        IPMI_DEVICE_SSIF,
                                        prog_data->args->common.disable_auto_probe,
                                        prog_data->args->common.driver_address,
                                        prog_data->args->common.register_spacing,
                                        prog_data->args->common.driver_device,
                                        prog_data->debug_flags) < 0)
                    {
                      fprintf(stderr,
                              "ipmi_open_inband: %s\n",
                              ipmi_device_strerror(ipmi_device_errnum(dev)));
                      exit_code = EXIT_FAILURE;
                      goto cleanup;
                    }
                }
            }
        }
      else
        {
          if (ipmi_open_inband (dev,
                                prog_data->args->common.driver_type,
                                prog_data->args->common.disable_auto_probe,
                                prog_data->args->common.driver_address,
                                prog_data->args->common.register_spacing,
                                prog_data->args->common.driver_device,
                                prog_data->debug_flags) < 0)
            {
              fprintf(stderr,
                      "ipmi_open_inband: %s\n",
                      ipmi_device_strerror(ipmi_device_errnum(dev)));
              exit_code = EXIT_FAILURE;
              goto cleanup;
            }
        }
    }
  
  _pef_config_state_data_init (&state_data);
  state_data.dev = dev;
  state_data.prog_data = prog_data;

  if (!(sections = pef_config_sections_list_create (&state_data)))
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  state_data.sections = sections;

  switch (prog_data->args->action) {
  case PEF_ACTION_INFO:
    ret = pef_info (&state_data);
    break;
  case PEF_ACTION_CHECKOUT:
    ret = pef_checkout (&state_data);
    break;
  case PEF_ACTION_COMMIT:
    ret = pef_commit (&state_data);
    break;
  case PEF_ACTION_DIFF:
    ret = pef_diff (&state_data);
    break;
  case PEF_ACTION_LIST_SECTIONS:
    ret = pef_config_sections_list (&state_data);
    break;
  }

  if (ret == PEF_ERR_FATAL_ERROR || ret == PEF_ERR_NON_FATAL_ERROR)
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }
  
  exit_code = 0;
 cleanup:
  if (dev)
    {
      ipmi_close_device (dev);
      ipmi_device_destroy (dev);
    }
  if (sections)
    pef_config_sections_list_destroy(&state_data, sections);
  return exit_code;
}

int 
main (int argc, char **argv)
{
  pef_config_prog_data_t prog_data;
  struct pef_config_arguments cmd_args;
  int exit_code;
  
  ipmi_disable_coredump();

  prog_data.progname = argv[0];
  pef_config_argp_parse (argc, argv, &cmd_args);

  if (pef_config_args_validate (&cmd_args) < 0)
    return (EXIT_FAILURE);

  prog_data.args = &cmd_args;

#ifndef NDEBUG
  if (prog_data.args->common.debug)
    prog_data.debug_flags = IPMI_FLAGS_DEBUG_DUMP;
  else
    prog_data.debug_flags = IPMI_FLAGS_DEFAULT;
#else  /* NDEBUG */
  prog_data.debug_flags = IPMI_FLAGS_DEFAULT;
#endif /* NDEBUG */
  
  exit_code = _pef_config (&prog_data);
  
  return exit_code;
}

