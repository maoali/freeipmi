/*
 * Copyright (C) 2008-2010 FreeIPMI Core Team
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

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <assert.h>

#include <freeipmi/freeipmi.h>

#include "ipmi-oem.h"
#include "ipmi-oem-argp.h"
#include "ipmi-oem-common.h"
#include "ipmi-oem-sun.h"

#include "freeipmi-portability.h"
#include "pstdout.h"
#include "tool-oem-common.h"
#include "tool-sdr-cache-common.h"
#include "tool-sensor-common.h"

/* IBM OEM SDR LED Record
 *
 * 1-2   - Record ID (standard)
 * 3     - Version (standard)
 * 4     - Record Type (0xC0 - standard)
 * 5     - Record Length (standard)
 * 6-8   - Manufacturer ID (standard)
 * 9-12  - ?? (oem_data)
 * 13    - OEM Sensor Type (oem_data)
 * 14-15 - ?? (oem_data)
 * 16-17 - LED ID (oem_data)
 */
#define IPMI_SDR_RECORD_OEM_IBM_LED_OEM_DATA_MIN_LENGTH    9
#define IPMI_SDR_RECORD_OEM_IBM_SENSOR_TYPE_OEM_DATA_INDEX 4
#define IPMI_SDR_RECORD_OEM_IBM_LED_ID_LS_OEM_DATA_INDEX   7
#define IPMI_SDR_RECORD_OEM_IBM_LED_ID_MS_OEM_DATA_INDEX   8

#define IPMI_IBM_LED_STATE_INACTIVE                  0x0

#define IPMI_IBM_LED_ACTIVE_BY_LED                   0x1
#define IPMI_IBM_LED_ACTIVE_BY_SENSOR                0x2
#define IPMI_IBM_LED_ACTIVE_BY_USER                  0x3
#define IPMI_IBM_LED_ACTIVE_BY_BIOS_OR_ADMINISTRATOR 0x4

#define IPMI_IBM_LED_X3455_LOCATION          0x1

#define IPMI_IBM_LED_X3755_CPU               0x0010
#define IPMI_IBM_LED_X3755_CPU1              0x0030
#define IPMI_IBM_LED_X3755_CPU2              0x0031
#define IPMI_IBM_LED_X3755_CPU3              0x0032
#define IPMI_IBM_LED_X3755_CPU4              0x0033

#define IPMI_IBM_LED_X3755_CPU1_BOARD        0x00B8
#define IPMI_IBM_LED_X3755_CPU2_BOARD        0x00B9
#define IPMI_IBM_LED_X3755_CPU3_BOARD        0x00BA
#define IPMI_IBM_LED_X3755_CPU4_BOARD        0x00BB

#define IPMI_IBM_LED_X3755_DIMM_1            0x0060
#define IPMI_IBM_LED_X3755_DIMM_2            0x0061
#define IPMI_IBM_LED_X3755_DIMM_3            0x0062 
#define IPMI_IBM_LED_X3755_DIMM_4            0x0063
#define IPMI_IBM_LED_X3755_DIMM_5            0x0064
#define IPMI_IBM_LED_X3755_DIMM_6            0x0065
#define IPMI_IBM_LED_X3755_DIMM_7            0x0066
#define IPMI_IBM_LED_X3755_DIMM_8            0x0067
#define IPMI_IBM_LED_X3755_DIMM_9            0x0068
#define IPMI_IBM_LED_X3755_DIMM_10           0x0069
#define IPMI_IBM_LED_X3755_DIMM_11           0x006A
#define IPMI_IBM_LED_X3755_DIMM_12           0x006B
#define IPMI_IBM_LED_X3755_DIMM_13           0x006C
#define IPMI_IBM_LED_X3755_DIMM_14           0x006D
#define IPMI_IBM_LED_X3755_DIMM_15           0x006E
#define IPMI_IBM_LED_X3755_DIMM_16           0x006F
#define IPMI_IBM_LED_X3755_DIMM_17           0x00C0
#define IPMI_IBM_LED_X3755_DIMM_18           0x00C1
#define IPMI_IBM_LED_X3755_DIMM_19           0x00C2
#define IPMI_IBM_LED_X3755_DIMM_20           0x00C3
#define IPMI_IBM_LED_X3755_DIMM_21           0x00C4
#define IPMI_IBM_LED_X3755_DIMM_22           0x00C5
#define IPMI_IBM_LED_X3755_DIMM_23           0x00C6
#define IPMI_IBM_LED_X3755_DIMM_24           0x00C7
#define IPMI_IBM_LED_X3755_DIMM_25           0x00C8
#define IPMI_IBM_LED_X3755_DIMM_26           0x00C9
#define IPMI_IBM_LED_X3755_DIMM_27           0x00CA
#define IPMI_IBM_LED_X3755_DIMM_28           0x00CB
#define IPMI_IBM_LED_X3755_DIMM_29           0x00CC
#define IPMI_IBM_LED_X3755_DIMM_30           0x00CD
#define IPMI_IBM_LED_X3755_DIMM_31           0x00CE
#define IPMI_IBM_LED_X3755_DIMM_32           0x00CF

#define IPMI_IBM_LED_X3755_FAN               0x0014
#define IPMI_IBM_LED_X3755_FAN_1             0x0050
#define IPMI_IBM_LED_X3755_FAN_2             0x0051
#define IPMI_IBM_LED_X3755_FAN_3             0x0052
#define IPMI_IBM_LED_X3755_FAN_4             0x0053
#define IPMI_IBM_LED_X3755_FAN_5             0x0054
#define IPMI_IBM_LED_X3755_FAN_6             0x0055
#define IPMI_IBM_LED_X3755_FAN_7             0x0056
#define IPMI_IBM_LED_X3755_FAN_8             0x0057

#define IPMI_IBM_LED_X3755_PCI               0x0020
#define IPMI_IBM_LED_X3755_PCI_1             0x0070
#define IPMI_IBM_LED_X3755_PCI_2             0x0071
#define IPMI_IBM_LED_X3755_PCI_3             0x0072
#define IPMI_IBM_LED_X3755_PCI_4             0x0073
#define IPMI_IBM_LED_X3755_PCI_5             0x0074
#define IPMI_IBM_LED_X3755_PCI_6             0x0075

#define IPMI_IBM_LED_X3755_SERVERAID_8K_BATT 0x00D0
#define IPMI_IBM_LED_X3755_SERVERAID_8K_ERR  0x00D1

#define IPMI_IBM_LED_X3755_ALERT             0x00D9
#define IPMI_IBM_LED_X3755_BK_BLUE           0x00D8
#define IPMI_IBM_LED_X3755_BOARD             0x000E
#define IPMI_IBM_LED_X3755_CNFG              0x0006
#define IPMI_IBM_LED_X3755_DASD              0x0013
#define IPMI_IBM_LED_X3755_FAULT             0x0000
#define IPMI_IBM_LED_X3755_HTX               0x00B0
#define IPMI_IBM_LED_X3755_INFO              0x0003
#define IPMI_IBM_LED_X3755_LOCATION          0x0001
#define IPMI_IBM_LED_X3755_MEM               0x0015
#define IPMI_IBM_LED_X3755_NMI               0x0019
#define IPMI_IBM_LED_X3755_OVERSPEC          0x001B
#define IPMI_IBM_LED_X3755_RAID              0x000F
#define IPMI_IBM_LED_X3755_SEER              0x000B
#define IPMI_IBM_LED_X3755_SP                0x001E
#define IPMI_IBM_LED_X3755_TEMP              0x001C
#define IPMI_IBM_LED_X3755_VRM               0x0011
 
#define IPMI_IBM_LED_X3755_UNKNOWN1          0x0040
#define IPMI_IBM_LED_X3755_UNKNOWN2          0x0041
#define IPMI_IBM_LED_X3755_UNKNOWN3          0x0047

#define IPMI_LED_NAME_COLUMN_SIZE  17
#define IPMI_LED_NAME_BUFLEN       17

#define IPMI_LED_STATE_COLUMN_SIZE 8

#define IPMI_LED_INFO_BUFLEN       1024

static int
_get_led_name (ipmi_oem_state_data_t *state_data,
               struct ipmi_oem_data *oem_data,
               uint16_t led_id,
               char *led_name,
               unsigned int led_name_len)
{
  char *led_id_str = NULL;

  assert (state_data);
  assert (oem_data);
  assert (led_name);
  assert (led_name_len);

   if (oem_data->manufacturer_id == IPMI_IANA_ENTERPRISE_ID_IBM
       && oem_data->product_id == IPMI_IBM_PRODUCT_ID_X3455)
     {
       if (led_id == IPMI_IBM_LED_X3455_LOCATION)
         led_id_str = "Location";
     }
   else if (oem_data->manufacturer_id == IPMI_IANA_ENTERPRISE_ID_IBM
            && oem_data->product_id == IPMI_IBM_PRODUCT_ID_X3755)
     {
       if (led_id == IPMI_IBM_LED_X3755_CPU)
         led_id_str = "CPU";
       else if (led_id == IPMI_IBM_LED_X3755_CPU1)
         led_id_str = "CPU1";
       else if (led_id == IPMI_IBM_LED_X3755_CPU2)
         led_id_str = "CPU2";
       else if (led_id == IPMI_IBM_LED_X3755_CPU3)
         led_id_str = "CPU3";
       else if (led_id == IPMI_IBM_LED_X3755_CPU4)
         led_id_str = "CPU4";
       else if (led_id == IPMI_IBM_LED_X3755_CPU1_BOARD)
         led_id_str = "CPU1_BOARD";
       else if (led_id == IPMI_IBM_LED_X3755_CPU2_BOARD)
         led_id_str = "CPU2_BOARD";
       else if (led_id == IPMI_IBM_LED_X3755_CPU3_BOARD)
         led_id_str = "CPU3_BOARD";
       else if (led_id == IPMI_IBM_LED_X3755_CPU4_BOARD)
         led_id_str = "CPU4_BOARD";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_1)
         led_id_str = "DIMM 1";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_2)
         led_id_str = "DIMM 2";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_3)
         led_id_str = "DIMM 3";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_4)
         led_id_str = "DIMM 4";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_5)
         led_id_str = "DIMM 5";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_6)
         led_id_str = "DIMM 6";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_7)
         led_id_str = "DIMM 7";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_8)
         led_id_str = "DIMM 8";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_9)
         led_id_str = "DIMM 9";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_10)
         led_id_str = "DIMM 10";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_11)
         led_id_str = "DIMM 11";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_12)
         led_id_str = "DIMM 12";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_13)
         led_id_str = "DIMM 13";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_14)
         led_id_str = "DIMM 14";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_15)
         led_id_str = "DIMM 15";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_16)
         led_id_str = "DIMM 16";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_17)
         led_id_str = "DIMM 17";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_18)
         led_id_str = "DIMM 18";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_19)
         led_id_str = "DIMM 19";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_20)
         led_id_str = "DIMM 20";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_21)
         led_id_str = "DIMM 21";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_22)
         led_id_str = "DIMM 22";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_23)
         led_id_str = "DIMM 23";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_24)
         led_id_str = "DIMM 24";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_25)
         led_id_str = "DIMM 25";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_26)
         led_id_str = "DIMM 26";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_27)
         led_id_str = "DIMM 27";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_28)
         led_id_str = "DIMM 28";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_29)
         led_id_str = "DIMM 29";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_30)
         led_id_str = "DIMM 30";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_31)
         led_id_str = "DIMM 31";
       else if (led_id == IPMI_IBM_LED_X3755_DIMM_32)
         led_id_str = "DIMM 32";
       else if (led_id == IPMI_IBM_LED_X3755_FAN)
         led_id_str = "FAN";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_1)
         led_id_str = "Fan 1";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_2)
         led_id_str = "Fan 2";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_3)
         led_id_str = "Fan 3";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_4)
         led_id_str = "Fan 4";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_5)
         led_id_str = "Fan 5";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_6)
         led_id_str = "Fan 6";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_7)
         led_id_str = "Fan 7";
       else if (led_id == IPMI_IBM_LED_X3755_FAN_8)
         led_id_str = "Fan 8";
       else if (led_id == IPMI_IBM_LED_X3755_PCI)
         led_id_str = "PCI";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_1)
         led_id_str = "PCI 1";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_2)
         led_id_str = "PCI 2";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_3)
         led_id_str = "PCI 3";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_4)
         led_id_str = "PCI 4";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_5)
         led_id_str = "PCI 5";
       else if (led_id == IPMI_IBM_LED_X3755_PCI_6)
         led_id_str = "PCI 6";
       else if (led_id == IPMI_IBM_LED_X3755_SERVERAID_8K_BATT)
         led_id_str = "ServeRAID 8k Batt";
       else if (led_id == IPMI_IBM_LED_X3755_SERVERAID_8K_ERR)
         led_id_str = "ServeRAID 8k Err";
       else if (led_id == IPMI_IBM_LED_X3755_ALERT)
         led_id_str = "Alert";
       else if (led_id == IPMI_IBM_LED_X3755_BK_BLUE)
         led_id_str = "BK_Blue";
       else if (led_id == IPMI_IBM_LED_X3755_BOARD)
         led_id_str = "BOARD";
       else if (led_id == IPMI_IBM_LED_X3755_CNFG)
         led_id_str = "CNFG";
       else if (led_id == IPMI_IBM_LED_X3755_DASD)
         led_id_str = "DASD";
       else if (led_id == IPMI_IBM_LED_X3755_FAULT)
         led_id_str = "FAULT";
       else if (led_id == IPMI_IBM_LED_X3755_HTX)
         led_id_str = "HTX";
       else if (led_id == IPMI_IBM_LED_X3755_INFO)
         led_id_str = "INFO";
       else if (led_id == IPMI_IBM_LED_X3755_LOCATION)
         led_id_str = "Location";
       else if (led_id == IPMI_IBM_LED_X3755_MEM)
         led_id_str = "MEM";
       else if (led_id == IPMI_IBM_LED_X3755_NMI)
         led_id_str = "NMI";
       else if (led_id == IPMI_IBM_LED_X3755_OVERSPEC)
         led_id_str = "OVERSPEC";
       else if (led_id == IPMI_IBM_LED_X3755_RAID)
         led_id_str = "RAID";
       else if (led_id == IPMI_IBM_LED_X3755_SEER)
         led_id_str = "SEER";
       else if (led_id == IPMI_IBM_LED_X3755_SP)
         led_id_str = "SP";
       else if (led_id == IPMI_IBM_LED_X3755_TEMP)
         led_id_str = "TEMP";
       else if (led_id == IPMI_IBM_LED_X3755_VRM)
         led_id_str = "VRM";
       else if (led_id == IPMI_IBM_LED_X3755_UNKNOWN1
                || led_id == IPMI_IBM_LED_X3755_UNKNOWN2
                || led_id == IPMI_IBM_LED_X3755_UNKNOWN3)
         led_id_str = "UNKNOWN";
     }

   if (led_id_str)
     snprintf (led_name,
               led_name_len,
               "%s",
               led_id_str);
   else
     snprintf (led_name,
               led_name_len,
               "LED = %04h",
               led_id);      
   
   return (0);
}

int
ipmi_oem_ibm_get_led (ipmi_oem_state_data_t *state_data)
{
  struct sensor_column_width column_width;
  struct ipmi_oem_data oem_data;
  uint16_t record_count;
  int rv = -1;
  int header_output_flag = 0;
  int i;
  
  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  if (sdr_cache_create_and_load (state_data->sdr_cache_ctx,
                                 state_data->pstate,
                                 state_data->ipmi_ctx,
                                 state_data->prog_data->args->sdr.quiet_cache,
                                 state_data->prog_data->args->sdr.sdr_cache_recreate,
                                 state_data->hostname,
                                 state_data->prog_data->args->sdr.sdr_cache_directory) < 0)
    goto cleanup;

  if (calculate_column_widths (state_data->pstate,
                               state_data->sdr_cache_ctx,
                               state_data->sdr_parse_ctx,
                               NULL,
                               0,
                               NULL,
                               0,
                               1, /* abbreviated_units */
                               0,
                               0,
                               0,
                               NULL,
                               &column_width) < 0)
    goto cleanup;

  if (ipmi_get_oem_data (state_data->pstate,
                         state_data->ipmi_ctx,
                         &oem_data) < 0)
    goto cleanup;

  /* IBM OEM
   *
   * From xCAT (http://xcat.sourceforge.net/)
   *
   * Get Led Request
   *
   * 0x3A - OEM network function (is IPMI_NET_FN_OEM_IBM_LED_RQ)
   * 0xC0 - OEM cmd
   * 0x?? - LED ID (MS Byte)
   * 0x?? - LED ID (LS Byte)
   *
   * Get Led Response
   *
   * 0xC0 - OEM cmd
   * 0x?? - Completion Code
   * 0x?? - ??
   * 0x?? - LED Active vs. Inactive
   *      - non-zero = active
   *      - 0 = inactive
   * 0x?? - ??
   * 0x?? - LED Pointer ID (MS Byte)
   * 0x?? - LED Pointer ID (LS Byte) / Sensor Number
   *      - Pointer ID means indicating problem elsewhere
   * 0x?? - LED Active Type
   *      - 1 - Indicates LED Active to indicate LED Pointer ID Active
   *      - 2 - Indicates LED Active due to Sensor w/ Sensor Number
   *      - 3 - User manually activated LED
   *      - 4 - BIOS or Administrator lit LED
   */

  if (ipmi_sdr_cache_record_count (state_data->sdr_cache_ctx, &record_count) < 0)
    {
      pstdout_fprintf (state_data->pstate,
		       stderr,
		       "ipmi_sdr_cache_record_count: %s\n",
		       ipmi_sdr_cache_ctx_errormsg (state_data->sdr_cache_ctx));
      goto cleanup;
    }

  for (i = 0; i < record_count; i++, ipmi_sdr_cache_next (state_data->sdr_cache_ctx))
    {
      uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
      uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
      int rs_len;
      uint8_t sdr_record[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
      int sdr_record_len = 0;
      uint8_t oem_data[IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH];
      int oem_data_len;
      uint16_t record_id;
      uint8_t record_type;
      char fmt[IPMI_OEM_FMT_BUFLEN + 1];
      char led_name[IPMI_LED_NAME_BUFLEN + 1];
      char led_pointer_name[IPMI_LED_NAME_BUFLEN + 1];
      char id_string[IPMI_SDR_CACHE_MAX_ID_STRING + 1];
      char led_info[IPMI_LED_INFO_BUFLEN + 1];
      char *led_state_str = NULL;
      uint8_t sensor_type;
      uint8_t led_id_ls;
      uint8_t led_id_ms;
      uint16_t led_id;
      uint8_t led_state;
      uint8_t led_active_type;
      uint16_t led_pointer_id;
      uint8_t sensor_number;

      if ((sdr_record_len = ipmi_sdr_cache_record_read (state_data->sdr_cache_ctx,
							sdr_record,
							IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
	{
	  pstdout_fprintf (state_data->pstate,
			   stderr,
			   "ipmi_sdr_cache_record_read: %s\n",
			   ipmi_sdr_cache_ctx_errormsg (state_data->sdr_cache_ctx));
	  goto cleanup;
	}
      
      if (ipmi_sdr_parse_record_id_and_type (state_data->sdr_parse_ctx,
					     sdr_record,
					     sdr_record_len,
					     &record_id,
					     &record_type) < 0)
	{
	  pstdout_fprintf (state_data->pstate,
			   stderr,
			   "ipmi_sdr_parse_record_id_and_type: %s\n",
			   ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
	  goto cleanup;
	}
      
      
      if (record_type != IPMI_SDR_FORMAT_OEM_RECORD)
        continue;

      if ((oem_data_len = ipmi_sdr_parse_oem_data (state_data->sdr_parse_ctx,
                                                   sdr_record,
                                                   sdr_record_len,
                                                   oem_data,
                                                   IPMI_SDR_CACHE_MAX_SDR_RECORD_LENGTH)) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sdr_parse_oem_data: %s\n",
                           ipmi_sdr_parse_ctx_errormsg (state_data->sdr_parse_ctx));
          goto cleanup;
        }
      
      /* If not enough data, continue on */
      if (oem_data_len < IPMI_SDR_RECORD_OEM_IBM_LED_OEM_DATA_MIN_LENGTH)
        continue;

      sensor_type = oem_data[IPMI_SDR_RECORD_OEM_IBM_SENSOR_TYPE_OEM_DATA_INDEX];
      
      /* IBM systems use inconsistent endian, guess endian by assuming
       * LED IDs are numerically started at 0
       */
      
      if (oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_LS_OEM_DATA_INDEX] > oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_MS_OEM_DATA_INDEX])
        {
          led_id_ls = oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_LS_OEM_DATA_INDEX];
          led_id_ms = oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_MS_OEM_DATA_INDEX];
        }
      else
        {
          led_id_ls = oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_MS_OEM_DATA_INDEX];
          led_id_ms = oem_data[IPMI_SDR_RECORD_OEM_IBM_LED_ID_LS_OEM_DATA_INDEX];
        }

      led_id = led_id_ls | (led_id_ms << 8);

      bytes_rq[0] = IPMI_CMD_OEM_IBM_GET_LED;
      bytes_rq[1] = led_id_ms;
      bytes_rq[2] = led_id_ls;
      
      if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                                  0, /* lun */
                                  IPMI_NET_FN_OEM_IBM_LED_RQ, /* network function */
                                  bytes_rq, /* data */
                                  3, /* num bytes */
                                  bytes_rs,
                                  IPMI_OEM_MAX_BYTES)) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_cmd_raw: %s\n",
                           ipmi_ctx_errormsg (state_data->ipmi_ctx));
          goto cleanup;
        }
      
      /* If get parameter out of range, assume LED ID endian wrong and try again */
      if (bytes_rs[1] == IPMI_COMP_CODE_PARAMETER_OUT_OF_RANGE)
        {
          bytes_rq[0] = IPMI_CMD_OEM_IBM_GET_LED;
          bytes_rq[1] = led_id_ls;
          bytes_rq[2] = led_id_ms;

          if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                                      0, /* lun */
                                      IPMI_NET_FN_OEM_IBM_LED_RQ, /* network function */
                                      bytes_rq, /* data */
                                      3, /* num bytes */
                                      bytes_rs,
                                      IPMI_OEM_MAX_BYTES)) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_cmd_raw: %s\n",
                               ipmi_ctx_errormsg (state_data->ipmi_ctx));
              goto cleanup;
            }
        }

      /* achu: there are probably 1 or 2 completion codes that are
       * acceptable to ignore and continue on, but who knows what they
       * are.
       */

      if (ipmi_oem_check_response_and_completion_code (state_data,
                                                       bytes_rs,
                                                       rs_len,
                                                       8,
                                                       IPMI_CMD_OEM_IBM_GET_LED,
                                                       IPMI_NET_FN_OEM_IBM_LED_RS,
                                                       NULL) < 0)
        goto cleanup;
      
      if (!header_output_flag)
        {
          memset (fmt, '\0', IPMI_OEM_FMT_BUFLEN + 1);
          
          snprintf (fmt,
                    IPMI_OEM_FMT_BUFLEN,
                    "%%-%ds | LED               | State   | LED Information\n",
                    column_width.record_id);

          pstdout_printf (state_data->pstate,
                          fmt,
                          SENSORS_HEADER_RECORD_ID_STR);
          
          header_output_flag++;
        }
      
      led_state = bytes_rs[3];
      led_active_type = bytes_rs[7];
      led_pointer_id = (bytes_rs[5] << 8) | bytes_rs[6];
      sensor_number = bytes_rs[6];
      
      memset (led_name, '\0', IPMI_LED_NAME_BUFLEN + 1);
      memset (led_pointer_name, '\0', IPMI_LED_NAME_BUFLEN + 1);
      memset (id_string, '\0', IPMI_SDR_CACHE_MAX_ID_STRING + 1);
      memset (led_info, '\0', IPMI_LED_INFO_BUFLEN + 1);
      
      if (_get_led_name (state_data,
                         &oem_data,
                         led_id,
                         led_name,
                         IPMI_LED_NAME_BUFLEN) < 0)
        goto cleanup;
      
      if (led_state == IPMI_IBM_LED_STATE_INACTIVE)
        led_state_str = "Inactive";
      else
        led_state_str = "Active";
      
      if (led_state != IPMI_IBM_LED_STATE_INACTIVE)
        {
          /* Location LED special case */
          if (!led_id)
            {
              snprintf (led_info,
                        IPMI_LED_INFO_BUFLEN,
                        "System Error Condition");
            }
          else if (led_active_type == IPMI_IBM_LED_ACTIVE_BY_LED)
            {
              if (_get_led_name (state_data,
                                 &oem_data,
                                 led_pointer_id,
                                 led_pointer_name,
                                 IPMI_LED_NAME_BUFLEN) < 0)
                goto cleanup;
              
              snprintf (led_info,
                        IPMI_LED_INFO_BUFLEN,
                        "'%s' Active",
                        led_pointer_name);
            }
          else if (led_active_type == IPMI_IBM_LED_ACTIVE_BY_SENSOR)
            {
              /* XXX todo */
            }
          else if (led_active_type == IPMI_IBM_LED_ACTIVE_BY_USER)
            {
              snprintf (led_info,
                        IPMI_LED_INFO_BUFLEN,
                        "LED Activated by User");
            }
          else if (led_active_type == IPMI_IBM_LED_ACTIVE_BY_BIOS_OR_ADMINISTRATOR)
            {
              snprintf (led_info,
                        IPMI_LED_INFO_BUFLEN,
                        "LED Activated by BIOS or Administrator");
            }
        }
      
      snprintf (fmt,
                IPMI_OEM_FMT_BUFLEN,
                "%%-%du | %%-%ds | %%-%ds | %s\n",
                column_width.record_id,
                IPMI_LED_NAME_COLUMN_SIZE,
                IPMI_LED_STATE_COLUMN_SIZE,
                led_info);
      
      pstdout_printf (state_data->pstate,
                      fmt,
                      record_id,
                      led_name,
                      led_state_str,
                      led_info);
    }
  
  
  rv = 0;
 cleanup:
  return (rv);
}