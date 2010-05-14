/*
 * Copyright (C) 2003-2010 FreeIPMI Core Team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#include "freeipmi/interpret/ipmi-interpret.h"

#include "freeipmi/record-format/ipmi-sdr-record-format.h"
#include "freeipmi/record-format/ipmi-sel-record-format.h"
#include "freeipmi/sel-parse/ipmi-sel-parse.h"
#include "freeipmi/spec/ipmi-event-reading-type-code-spec.h"
#include "freeipmi/spec/ipmi-sensor-types-spec.h"

#include "ipmi-interpret-defs.h"
#include "ipmi-interpret-trace.h"
#include "ipmi-interpret-config.h"
#include "ipmi-interpret-util.h"

#include "libcommon/ipmi-fiid-util.h"

#include "freeipmi-portability.h"

static char *ipmi_interpret_errmsgs[] =
  {
    "success",
    "context null",
    "context invalid",
    "invalid parameters",
    "out of memory",
    "permission denied",
    "sel config file does not exist",
    "sel config file parse error",
    "sensor config file does not exist",
    "sensor config file parse error",
    "invalid sel record",
    "internal system error",
    "buffer overflow",
    "internal error",
    "errnum out of range",
    NULL
  };

/* 16th bit reserved, see Get Sensor Reading command in spec */
#define IPMI_INTERPRET_MAX_SENSOR_AND_EVENT_OFFSET 15

ipmi_interpret_ctx_t
ipmi_interpret_ctx_create (void)
{
  struct ipmi_interpret_ctx *ctx = NULL;

  if (!(ctx = (ipmi_interpret_ctx_t)malloc (sizeof (struct ipmi_interpret_ctx))))
    {
      ERRNO_TRACE (errno);
      return (NULL);
    }
  memset (ctx, '\0', sizeof (struct ipmi_interpret_ctx));
  ctx->magic = IPMI_INTERPRET_CTX_MAGIC;
  ctx->flags = IPMI_INTERPRET_FLAGS_DEFAULT;

  if (!(ctx->sel_parse_ctx = ipmi_sel_parse_ctx_create (NULL, NULL)))
    {
      ERRNO_TRACE (errno);
      goto cleanup;
    }

  if (ipmi_interpret_sel_init (ctx) < 0)
    goto cleanup;

  if (ipmi_interpret_sensor_init (ctx) < 0)
    goto cleanup;

  return (ctx);

 cleanup:
  if (ctx)
    {
      ipmi_sel_parse_ctx_destroy (ctx->sel_parse_ctx);
      ipmi_interpret_sel_destroy (ctx);
      ipmi_interpret_sensor_destroy (ctx);
      free (ctx);
    }
  return (NULL);
}

void
ipmi_interpret_ctx_destroy (ipmi_interpret_ctx_t ctx)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    return;

  ipmi_sel_parse_ctx_destroy (ctx->sel_parse_ctx);
  ipmi_interpret_sel_destroy (ctx);
  ipmi_interpret_sensor_destroy (ctx);

  ctx->magic = ~IPMI_INTERPRET_CTX_MAGIC;
  free (ctx);
}

int
ipmi_interpret_ctx_errnum (ipmi_interpret_ctx_t ctx)
{
  if (!ctx)
    return (IPMI_INTERPRET_ERR_CONTEXT_NULL);
  else if (ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    return (IPMI_INTERPRET_ERR_CONTEXT_INVALID);
  else
    return (ctx->errnum);
}

char *
ipmi_interpret_ctx_strerror (int errnum)
{
  if (errnum >= IPMI_INTERPRET_ERR_SUCCESS && errnum <= IPMI_INTERPRET_ERR_ERRNUMRANGE)
    return (ipmi_interpret_errmsgs[errnum]);
  else
    return (ipmi_interpret_errmsgs[IPMI_INTERPRET_ERR_ERRNUMRANGE]);
}

char *
ipmi_interpret_ctx_errormsg (ipmi_interpret_ctx_t ctx)
{
  return (ipmi_interpret_ctx_strerror (ipmi_interpret_ctx_errnum (ctx)));
}

int
ipmi_interpret_ctx_get_flags (ipmi_interpret_ctx_t ctx, unsigned int *flags)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (!flags)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  *flags = ctx->flags;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_ctx_set_flags (ipmi_interpret_ctx_t ctx, unsigned int flags)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (flags & ~IPMI_INTERPRET_FLAGS_MASK)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  ctx->flags = flags;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_ctx_get_manufacturer_id (ipmi_interpret_ctx_t ctx, uint32_t *manufacturer_id)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (!manufacturer_id)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  *manufacturer_id = ctx->manufacturer_id;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_ctx_set_manufacturer_id (ipmi_interpret_ctx_t ctx, uint32_t manufacturer_id)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  ctx->manufacturer_id = manufacturer_id;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_ctx_get_product_id (ipmi_interpret_ctx_t ctx, uint16_t *product_id)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (!product_id)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  *product_id = ctx->product_id;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_ctx_set_product_id (ipmi_interpret_ctx_t ctx, uint16_t product_id)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  ctx->product_id = product_id;
  ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  return (0);
}

int
ipmi_interpret_load_sel_config (ipmi_interpret_ctx_t ctx,
                                const char *sel_config_file)
{
  struct stat buf;
  int rv = -1;

  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (sel_config_file)
    {
      if (stat (sel_config_file, &buf) < 0)
        {
          if (errno == EACCES || errno == EPERM)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PERMISSION);
          else if (errno == ENOENT)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_SEL_CONFIG_FILE_DOES_NOT_EXIST);
          else
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
          goto cleanup;
        }
    }
  else
    {
      if (stat (INTERPRET_SEL_CONFIG_FILE_DEFAULT, &buf) < 0)
        {
          /* Its not an error if the default configuration file doesn't exist */
          if (errno == ENOENT)
            goto out;
          else if (errno == EACCES || errno == EPERM)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PERMISSION);
          else
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
          goto cleanup;
        }
    }

  if (ipmi_interpret_sel_config_parse (ctx, sel_config_file) < 0)
    goto cleanup;

 out:
  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_interpret_load_sensor_config (ipmi_interpret_ctx_t ctx,
                                   const char *sensor_config_file)
{
  struct stat buf;
  int rv = -1;

  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (sensor_config_file)
    {
      if (stat (sensor_config_file, &buf) < 0)
        {
          if (errno == EACCES || errno == EPERM)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PERMISSION);
          else if (errno == ENOENT)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_SENSOR_CONFIG_FILE_DOES_NOT_EXIST);
          else
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
          goto cleanup;
        }
    }
  else
    {
      if (stat (INTERPRET_SENSOR_CONFIG_FILE_DEFAULT, &buf) < 0)
        {
          /* Its not an error if the default configuration file doesn't exist */
          if (errno == ENOENT)
            goto out;
          else if (errno == EACCES || errno == EPERM)
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PERMISSION);
          else
            INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
          goto cleanup;
        }
    }

  if (ipmi_interpret_sensor_config_parse (ctx, sensor_config_file) < 0)
    goto cleanup;

 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
_get_sel_state (ipmi_interpret_ctx_t ctx,
                uint8_t offset_from_event_reading_type_code,
                unsigned int *sel_state,
                struct ipmi_interpret_config **config)
{
  int i = 0;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (sel_state);
  assert (config);

  (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;

  i = 0;
  while (config[i]
         && i < offset_from_event_reading_type_code
         && i < IPMI_INTERPRET_MAX_SENSOR_AND_EVENT_OFFSET)
    i++;

  if (config[i])
    (*sel_state) = config[i]->state;

  return (0);
}

static int
_get_sel_oem_sensor_state (ipmi_interpret_ctx_t ctx,
                           const void *record_buf,
                           unsigned int record_buflen,
                           uint8_t event_reading_type_code,
                           uint8_t sensor_type,
                           unsigned int *sel_state)
{
  char keybuf[IPMI_OEM_HASH_KEY_BUFLEN + 1];
  struct ipmi_interpret_sel_oem_sensor_config *oem_conf;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (record_buf);
  assert (record_buflen);
  assert (sel_state);

  memset (keybuf, '\0', IPMI_OEM_HASH_KEY_BUFLEN + 1);

  snprintf (keybuf,
            IPMI_OEM_HASH_KEY_BUFLEN,
            "%u:%u:%u:%u",
            ctx->manufacturer_id,
            ctx->product_id,
            event_reading_type_code,
            sensor_type);
  
  if ((oem_conf = hash_find (ctx->interpret_sel.sel_oem_sensor_config,
			     keybuf)))
    {
      unsigned int i;
      uint8_t event_direction;
      uint8_t event_data1;
      uint8_t event_data2;
      uint8_t event_data3;
      int found = 0;
      
      (*sel_state) = IPMI_INTERPRET_STATE_NOMINAL;
      
      if (ipmi_sel_parse_record_sensor_type (ctx->sel_parse_ctx,
                                             record_buf,
                                             record_buflen,
                                             &sensor_type) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }

      if (ipmi_sel_parse_record_event_direction (ctx->sel_parse_ctx,
                                                 record_buf,
                                                 record_buflen,
                                                 &event_direction) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }

      if (ipmi_sel_parse_record_event_data1 (ctx->sel_parse_ctx,
                                             record_buf,
                                             record_buflen,
                                             &event_data1) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }

      if (ipmi_sel_parse_record_event_data2 (ctx->sel_parse_ctx,
                                             record_buf,
                                             record_buflen,
                                             &event_data2) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }
      
      if (ipmi_sel_parse_record_event_data3 (ctx->sel_parse_ctx,
                                             record_buf,
                                             record_buflen,
                                             &event_data3) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }

      for (i = 0; i < oem_conf->oem_sensor_data_count; i++)
	{
          
	  if ((oem_conf->oem_sensor_data[i].event_direction_any_flag
               || oem_conf->oem_sensor_data[i].event_direction == event_direction)
              && (oem_conf->oem_sensor_data[i].event_data1_any_flag
                  || oem_conf->oem_sensor_data[i].event_data1 == event_data1)
              && (oem_conf->oem_sensor_data[i].event_data2_any_flag
                  || oem_conf->oem_sensor_data[i].event_data2 == event_data2)
              && (oem_conf->oem_sensor_data[i].event_data3_any_flag
                  || oem_conf->oem_sensor_data[i].event_data3 == event_data3))
	    {
              if (oem_conf->oem_sensor_data[i].sel_state > (*sel_state))
                (*sel_state) = oem_conf->oem_sensor_data[i].sel_state;
              found++;
	    }
	}
      
      if (!found)
	(*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
    }
  else
    (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;

  return (0);
}

static int
_get_sel_oem_record_state (ipmi_interpret_ctx_t ctx,
                           const void *record_buf,
                           unsigned int record_buflen,
                           uint8_t record_type,
                           unsigned int *sel_state)
{
  char keybuf[IPMI_OEM_HASH_KEY_BUFLEN + 1];
  struct ipmi_interpret_sel_oem_record_config *oem_conf;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (record_buf);
  assert (record_buflen);
  assert (sel_state);

  memset (keybuf, '\0', IPMI_OEM_HASH_KEY_BUFLEN + 1);

  snprintf (keybuf,
            IPMI_OEM_HASH_KEY_BUFLEN,
            "%u:%u:%u",
            ctx->manufacturer_id,
            ctx->product_id,
            record_type);
  
  if ((oem_conf = hash_find (ctx->interpret_sel.sel_oem_record_config,
			     keybuf)))
    {
      unsigned int i, j;
      uint8_t oem_data[IPMI_SEL_OEM_DATA_MAX];
      int oem_data_len;
      int found = 0;
      
      (*sel_state) = IPMI_INTERPRET_STATE_NOMINAL;
      
      if ((oem_data_len = ipmi_sel_parse_record_oem (ctx->sel_parse_ctx,
                                                     record_buf,
                                                     record_buflen,
                                                     oem_data,
                                                     IPMI_SEL_OEM_DATA_MAX)) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          return (-1);
        }

      for (i = 0; i < oem_conf->oem_record_count; i++)
	{
          int match = 1;
          
          if (oem_data_len != oem_conf->oem_record[i].oem_bytes_count)
            continue;

          for (j = 0; j < oem_conf->oem_record[i].oem_bytes_count; j++)
            {
              if (!oem_conf->oem_record[i].oem_bytes[j].any_flag
                  && oem_conf->oem_record[i].oem_bytes[j].oem_data_byte != oem_data[j])
                {
                  match = 0;
                  break;
                }
            }
          
          if (match)
            {
              if (oem_conf->oem_record[i].sel_state > (*sel_state))
                (*sel_state) = oem_conf->oem_record[i].sel_state;
              found++;
            }
	}
      
      if (!found)
	(*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
    }
  else
    (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;

  return (0);
}

int
ipmi_interpret_sel (ipmi_interpret_ctx_t ctx,
                    const void *record_buf,
                    unsigned int record_buflen,
                    unsigned int *sel_state)
{
  struct ipmi_interpret_config **sel_config = NULL;
  uint8_t record_type;
  int rv = -1;

  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (!sel_state)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  if (ipmi_sel_parse_record_record_type (ctx->sel_parse_ctx,
                                         record_buf,
                                         record_buflen,
                                         &record_type) < 0)
    {
      INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
      return (-1);
    }

  /* IPMI Workaround
   *
   * HP DL 380 G5
   *
   * Motherboard is reporting SEL Records of record type 0x00, which
   * is not a valid record type.
   */
  if (ctx->flags & IPMI_INTERPRET_FLAGS_SEL_ASSUME_SYSTEM_EVENT_RECORDS
      && !IPMI_SEL_RECORD_TYPE_VALID (record_type))
    record_type = IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD;

  if (record_type == IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD)
    {
      uint8_t event_reading_type_code;
      uint8_t sensor_type;
      uint8_t offset_from_event_reading_type_code;

      if (ipmi_sel_parse_record_sensor_type (ctx->sel_parse_ctx,
                                             record_buf,
                                             record_buflen,
                                             &sensor_type) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          goto cleanup;
        }

      if (ipmi_sel_parse_record_event_type_code (ctx->sel_parse_ctx,
                                                 record_buf,
                                                 record_buflen,
                                                 &event_reading_type_code) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          goto cleanup;
        }

      if (ipmi_sel_parse_record_event_data1_offset_from_event_reading_type_code (ctx->sel_parse_ctx,
                                                                                 record_buf,
                                                                                 record_buflen,
                                                                                 &offset_from_event_reading_type_code) < 0)
        {
          INTERPRET_SEL_PARSE_CTX_ERROR_TO_INTERPRET_ERRNUM (ctx, ctx->sel_parse_ctx);
          goto cleanup;
        }

      if (IPMI_EVENT_READING_TYPE_CODE_IS_THRESHOLD (event_reading_type_code))
        {
          if (_get_sel_state (ctx,
                              offset_from_event_reading_type_code,
                              sel_state,
                              ctx->interpret_sel.ipmi_interpret_threshold_sel_config) < 0)
            goto cleanup;
        }
      else if (IPMI_EVENT_READING_TYPE_CODE_IS_GENERIC (event_reading_type_code))
        {
          if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
              && sensor_type == IPMI_SENSOR_TYPE_VOLTAGE)
            sel_config = ctx->interpret_sel.ipmi_interpret_voltage_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_PERFORMANCE
                   && sensor_type == IPMI_SENSOR_TYPE_VOLTAGE)
            sel_config = ctx->interpret_sel.ipmi_interpret_voltage_performance_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
                   && sensor_type == IPMI_SENSOR_TYPE_FAN)
            sel_config = ctx->interpret_sel.ipmi_interpret_fan_device_present_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_TRANSITION_AVAILABILITY
                   && sensor_type == IPMI_SENSOR_TYPE_FAN)
            sel_config = ctx->interpret_sel.ipmi_interpret_fan_transition_availability_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
                   && sensor_type == IPMI_SENSOR_TYPE_FAN)
            sel_config = ctx->interpret_sel.ipmi_interpret_fan_redundancy_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
                   && sensor_type == IPMI_SENSOR_TYPE_PROCESSOR)
            sel_config = ctx->interpret_sel.ipmi_interpret_processor_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
                   && sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_supply_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
                   && sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_supply_redundancy_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
                   && sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_unit_device_present_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
                   && sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_unit_redundancy_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
                   && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
            sel_config = ctx->interpret_sel.ipmi_interpret_drive_slot_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_PREDICTIVE_FAILURE
                   && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
            sel_config = ctx->interpret_sel.ipmi_interpret_drive_slot_predictive_failure_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
                   && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
            sel_config = ctx->interpret_sel.ipmi_interpret_drive_slot_device_present_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
                   && sensor_type == IPMI_SENSOR_TYPE_BUTTON_SWITCH)
            sel_config = ctx->interpret_sel.ipmi_interpret_button_switch_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
                   && sensor_type == IPMI_SENSOR_TYPE_MODULE_BOARD)
            sel_config = ctx->interpret_sel.ipmi_interpret_module_board_state_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
                   && sensor_type == IPMI_SENSOR_TYPE_MODULE_BOARD)
            sel_config = ctx->interpret_sel.ipmi_interpret_module_board_device_present_config;
          else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
                   && sensor_type == IPMI_SENSOR_TYPE_ENTITY_PRESENCE)
            sel_config = ctx->interpret_sel.ipmi_interpret_entity_presence_device_present_config;
          else
            {
              (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
              rv = 0;
              goto cleanup;
            }

          if (_get_sel_state (ctx,
                              offset_from_event_reading_type_code,
                              sel_state,
                              sel_config) < 0)
            goto cleanup;

        }
      else if (IPMI_EVENT_READING_TYPE_CODE_IS_SENSOR_SPECIFIC (event_reading_type_code))
        {
          if (sensor_type == IPMI_SENSOR_TYPE_PHYSICAL_SECURITY)
            sel_config = ctx->interpret_sel.ipmi_interpret_physical_security_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_PLATFORM_SECURITY_VIOLATION_ATTEMPT)
            sel_config = ctx->interpret_sel.ipmi_interpret_platform_security_violation_attempt_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_PROCESSOR)
            sel_config = ctx->interpret_sel.ipmi_interpret_processor_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_supply_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
            sel_config = ctx->interpret_sel.ipmi_interpret_power_unit_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_MEMORY)
            sel_config = ctx->interpret_sel.ipmi_interpret_memory_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
            sel_config = ctx->interpret_sel.ipmi_interpret_drive_slot_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS)
            sel_config = ctx->interpret_sel.ipmi_interpret_system_firmware_progress_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_EVENT_LOGGING_DISABLED)
            sel_config = ctx->interpret_sel.ipmi_interpret_event_logging_disabled_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_EVENT)
            sel_config = ctx->interpret_sel.ipmi_interpret_system_event_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT)
            sel_config = ctx->interpret_sel.ipmi_interpret_critical_interrupt_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_BUTTON_SWITCH)
            sel_config = ctx->interpret_sel.ipmi_interpret_button_switch_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_CHIP_SET)
            sel_config = ctx->interpret_sel.ipmi_interpret_chip_set_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_CABLE_INTERCONNECT)
            sel_config = ctx->interpret_sel.ipmi_interpret_cable_interconnect_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_BOOT_INITIATED)
            sel_config = ctx->interpret_sel.ipmi_interpret_system_boot_initiated_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_BOOT_ERROR)
            sel_config = ctx->interpret_sel.ipmi_interpret_boot_error_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_OS_BOOT)
            sel_config = ctx->interpret_sel.ipmi_interpret_os_boot_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_OS_CRITICAL_STOP)
            sel_config = ctx->interpret_sel.ipmi_interpret_os_critical_stop_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SLOT_CONNECTOR)
            sel_config = ctx->interpret_sel.ipmi_interpret_slot_connector_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_ACPI_POWER_STATE)
            sel_config = ctx->interpret_sel.ipmi_interpret_system_acpi_power_state_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_WATCHDOG2)
            sel_config = ctx->interpret_sel.ipmi_interpret_watchdog2_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_PLATFORM_ALERT)
            sel_config = ctx->interpret_sel.ipmi_interpret_platform_alert_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_ENTITY_PRESENCE)
            sel_config = ctx->interpret_sel.ipmi_interpret_entity_presence_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_LAN)
            sel_config = ctx->interpret_sel.ipmi_interpret_lan_config;       
          else if (sensor_type == IPMI_SENSOR_TYPE_MANAGEMENT_SUBSYSTEM_HEALTH)
            sel_config = ctx->interpret_sel.ipmi_interpret_management_subsystem_health_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_BATTERY)
            sel_config = ctx->interpret_sel.ipmi_interpret_battery_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_SESSION_AUDIT)
            sel_config = ctx->interpret_sel.ipmi_interpret_session_audit_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_VERSION_CHANGE)
            sel_config = ctx->interpret_sel.ipmi_interpret_version_change_config;
          else if (sensor_type == IPMI_SENSOR_TYPE_FRU_STATE)
            sel_config = ctx->interpret_sel.ipmi_interpret_fru_state_config;
          else if (ctx->flags & IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA
                   && IPMI_SENSOR_TYPE_IS_OEM (sensor_type))
            {
              if (_get_sel_oem_sensor_state (ctx,
                                             record_buf,
                                             record_buflen,
                                             event_reading_type_code,
                                             sensor_type,
                                             sel_state) < 0)
                goto cleanup;
              rv = 0;
              goto cleanup;
            }
          else
            {
              (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
              rv = 0;
              goto cleanup;
            }

          if (_get_sel_state (ctx,
                              offset_from_event_reading_type_code,
                              sel_state,
                              sel_config) < 0)
            goto cleanup;
        }
      else if (ctx->flags & IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA
               && IPMI_EVENT_READING_TYPE_CODE_IS_OEM (event_reading_type_code))
        {
          if (_get_sel_oem_sensor_state (ctx,
                                         record_buf,
                                         record_buflen,
                                         event_reading_type_code,
                                         sensor_type,
                                         sel_state) < 0)
            goto cleanup;
        }
      else
        {
          (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
          rv = 0;
          goto cleanup;
        }
    }
  else if (ctx->flags & IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA)
    {
      if (_get_sel_oem_record_state (ctx,
                                     record_buf,
                                     record_buflen,
                                     record_type,
                                     sel_state) < 0)
        goto cleanup;
    }
  else
    {
      (*sel_state) = IPMI_INTERPRET_STATE_UNKNOWN;
      rv = 0;
      goto cleanup;
    }

  rv = 0;
 cleanup:
  return (rv);
}

static int
_get_threshold_sensor_state (ipmi_interpret_ctx_t ctx,
                             uint16_t sensor_event_bitmask,
                             unsigned int *sensor_state)
{
  struct ipmi_interpret_config **config;
  int i = 0;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (sensor_state);
  
  config = ctx->interpret_sensor.ipmi_interpret_threshold_sensor_config;
  
  (*sensor_state) = IPMI_INTERPRET_STATE_NOMINAL;
  
  i = 0;
  while (config[i] && i < IPMI_INTERPRET_MAX_SENSOR_AND_EVENT_OFFSET)
    {
      if (sensor_event_bitmask & (0x1 << i))
        {
          if (config[i]->state > (*sensor_state))
            (*sensor_state) = config[i]->state;
        }
      
      sensor_event_bitmask &= ~(0x1 << i);
      i++;
    }
  
  /* remaining bits may be set to 1b as defined by IPMI spec, ignore them */
  
  return (0);
}

static int
_get_sensor_state (ipmi_interpret_ctx_t ctx,
                   uint16_t sensor_event_bitmask,
                   unsigned int *sensor_state,
                   struct ipmi_interpret_config **config)
{
  int i = 0;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (sensor_state);
  assert (config);

  (*sensor_state) = IPMI_INTERPRET_STATE_NOMINAL;

  i = 0;
  while (config[i] && i < IPMI_INTERPRET_MAX_SENSOR_AND_EVENT_OFFSET)
    {
      if (sensor_event_bitmask & (0x1 << i))
        {
          if (config[i]->state > (*sensor_state))
            (*sensor_state) = config[i]->state;
        }

      sensor_event_bitmask &= ~(0x1 << i);
      i++;
    }

  /* ignore 16th bit, as specified in IPMI spec */
  sensor_event_bitmask &= ~(0x1 << IPMI_INTERPRET_MAX_SENSOR_AND_EVENT_OFFSET);

  /* if any bits still set, they are outside of specification range */
  if (sensor_event_bitmask)
    (*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;

  return (0);
}

static int
_get_sensor_oem_state (ipmi_interpret_ctx_t ctx,
		       uint8_t event_reading_type_code,
		       uint8_t sensor_type,
		       uint16_t sensor_event_bitmask,
		       unsigned int *sensor_state)
{
  char keybuf[IPMI_OEM_HASH_KEY_BUFLEN + 1];
  struct ipmi_interpret_sensor_oem_config *oem_conf;

  assert (ctx);
  assert (ctx->magic == IPMI_INTERPRET_CTX_MAGIC);
  assert (sensor_state);

  memset (keybuf, '\0', IPMI_OEM_HASH_KEY_BUFLEN + 1);

  snprintf (keybuf,
            IPMI_OEM_HASH_KEY_BUFLEN,
            "%u:%u:%u:%u",
            ctx->manufacturer_id,
            ctx->product_id,
            event_reading_type_code,
            sensor_type);

  if ((oem_conf = hash_find (ctx->interpret_sensor.sensor_oem_config,
			     keybuf)))
    {
      unsigned int i;
      int found = 0;

      (*sensor_state) = IPMI_INTERPRET_STATE_NOMINAL;

      for (i = 0; i < oem_conf->oem_state_count; i++)
	{
	  if (oem_conf->oem_state[i].oem_state_type == IPMI_OEM_STATE_TYPE_BITMASK)
	    {
	      if (oem_conf->oem_state[i].sensor_event_bitmask & sensor_event_bitmask)
		{
		  if (oem_conf->oem_state[i].sensor_state > (*sensor_state))
		    (*sensor_state) = oem_conf->oem_state[i].sensor_state;
		  found++;
		}
	    }
	  else
	    {
	      if (oem_conf->oem_state[i].sensor_event_bitmask == sensor_event_bitmask)
		{
		  if (oem_conf->oem_state[i].sensor_state > (*sensor_state))
		    (*sensor_state) = oem_conf->oem_state[i].sensor_state;
		  found++;
		}
	    }
	}
      
      if (!found)
	(*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;
    }
  else
    (*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;

  return (0);
}

int
ipmi_interpret_sensor (ipmi_interpret_ctx_t ctx,
                       uint8_t event_reading_type_code,
                       uint8_t sensor_type,
                       uint16_t sensor_event_bitmask,
                       unsigned int *sensor_state)
{
  struct ipmi_interpret_config **sensor_config = NULL;
  int rv = -1;

  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    {
      ERR_TRACE (ipmi_interpret_ctx_errormsg (ctx), ipmi_interpret_ctx_errnum (ctx));
      return (-1);
    }

  if (!sensor_state)
    {
      INTERPRET_SET_ERRNUM (ctx, IPMI_INTERPRET_ERR_PARAMETERS);
      return (-1);
    }

  if (IPMI_EVENT_READING_TYPE_CODE_IS_THRESHOLD (event_reading_type_code))
    {
      if (_get_threshold_sensor_state (ctx,
                                       sensor_event_bitmask,
                                       sensor_state) < 0)
        goto cleanup;
    }
  else if (IPMI_EVENT_READING_TYPE_CODE_IS_GENERIC (event_reading_type_code))
    {
      if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
          && sensor_type == IPMI_SENSOR_TYPE_VOLTAGE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_voltage_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_PERFORMANCE
               && sensor_type == IPMI_SENSOR_TYPE_VOLTAGE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_voltage_performance_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
               && sensor_type == IPMI_SENSOR_TYPE_FAN)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_fan_device_present_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_TRANSITION_AVAILABILITY
               && sensor_type == IPMI_SENSOR_TYPE_FAN)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_fan_transition_availability_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
               && sensor_type == IPMI_SENSOR_TYPE_FAN)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_fan_redundancy_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
               && sensor_type == IPMI_SENSOR_TYPE_PROCESSOR)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_processor_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
               && sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_supply_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
               && sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_supply_redundancy_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
               && sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_unit_device_present_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_REDUNDANCY
               && sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_unit_redundancy_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
               && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_drive_slot_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_PREDICTIVE_FAILURE
               && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_drive_slot_predictive_failure_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
               && sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_drive_slot_device_present_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
               && sensor_type == IPMI_SENSOR_TYPE_BUTTON_SWITCH)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_button_switch_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_STATE
               && sensor_type == IPMI_SENSOR_TYPE_MODULE_BOARD)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_module_board_state_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
               && sensor_type == IPMI_SENSOR_TYPE_MODULE_BOARD)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_module_board_device_present_config;
      else if (event_reading_type_code == IPMI_EVENT_READING_TYPE_CODE_DEVICE_PRESENT
               && sensor_type == IPMI_SENSOR_TYPE_ENTITY_PRESENCE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_entity_presence_device_present_config;
      else
        {
          (*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;
          rv = 0;
          goto cleanup;
        }

      if (_get_sensor_state (ctx,
                             sensor_event_bitmask,
                             sensor_state,
                             sensor_config) < 0)
        goto cleanup;

    }
  else if (IPMI_EVENT_READING_TYPE_CODE_IS_SENSOR_SPECIFIC (event_reading_type_code))
    {
      if (sensor_type == IPMI_SENSOR_TYPE_PHYSICAL_SECURITY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_physical_security_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_PLATFORM_SECURITY_VIOLATION_ATTEMPT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_platform_security_violation_attempt_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_PROCESSOR)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_processor_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_POWER_SUPPLY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_supply_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_POWER_UNIT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_power_unit_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_MEMORY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_memory_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_DRIVE_SLOT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_drive_slot_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_system_firmware_progress_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_EVENT_LOGGING_DISABLED)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_event_logging_disabled_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_EVENT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_system_event_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_critical_interrupt_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_BUTTON_SWITCH)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_button_switch_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_CABLE_INTERCONNECT)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_cable_interconnect_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_BOOT_ERROR)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_boot_error_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_SLOT_CONNECTOR)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_slot_connector_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_SYSTEM_ACPI_POWER_STATE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_system_acpi_power_state_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_WATCHDOG2)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_watchdog2_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_ENTITY_PRESENCE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_entity_presence_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_MANAGEMENT_SUBSYSTEM_HEALTH)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_management_subsystem_health_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_BATTERY)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_battery_config;
      else if (sensor_type == IPMI_SENSOR_TYPE_FRU_STATE)
        sensor_config = ctx->interpret_sensor.ipmi_interpret_fru_state_config;
      else if (ctx->flags & IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA
	       && IPMI_SENSOR_TYPE_IS_OEM (sensor_type))
	{
	  if (_get_sensor_oem_state (ctx,
				     event_reading_type_code,
				     sensor_type,
				     sensor_event_bitmask,
				     sensor_state) < 0)
	    goto cleanup;
          rv = 0;
          goto cleanup;
	}
      else
        {
          (*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;
          rv = 0;
          goto cleanup;
        }

      if (_get_sensor_state (ctx,
                             sensor_event_bitmask,
                             sensor_state,
                             sensor_config) < 0)
        goto cleanup;
    }
  else if (ctx->flags & IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA
	   && IPMI_EVENT_READING_TYPE_CODE_IS_OEM (event_reading_type_code))
    {
      if (_get_sensor_oem_state (ctx,
				 event_reading_type_code,
				 sensor_type,
				 sensor_event_bitmask,
				 sensor_state) < 0)
	goto cleanup;
    }
  else
    {
      (*sensor_state) = IPMI_INTERPRET_STATE_UNKNOWN;
      rv = 0;
      goto cleanup;
    }

  rv = 0;
 cleanup:
  return (rv);
}
