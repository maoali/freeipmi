/* 
   ipmi-utils.c - general utility procedures

   Copyright (C) 2003, 2004, 2005 FreeIPMI Core Team

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  

*/

/* 2's complement checksum of preceding bytes in the connection header
   or between the previous checksum. 8-bit checksum algorithm:
   Initialize checksum to 0. 
   For each byte, checksum = (checksum + byte) modulo 256. Then find
   1's compliment of checksum and add one to it.
   To verify add all the bytes and the checksum and then % 256 should
   yield 0.
*/

#include "freeipmi.h"

int8_t
ipmi_chksum (uint8_t *buf, uint64_t len)
{
  register uint64_t i = 0;
  register int8_t chksum = 0;
 
  if (buf == NULL || len == 0)
    return (chksum);

  for (; i < len; i++)
    chksum = (chksum + buf[i]) % 256;

  return (-chksum);
}

int8_t
ipmi_chksum_test (uint8_t *buf, uint64_t len) 
{
  int8_t chksum_val;
  int8_t chksum_calc;

  if (buf == NULL || len == 0)
    {
      errno = EINVAL;
      return (-1);
    }

  chksum_val = buf[len - 1];
  chksum_calc = ipmi_chksum(buf, len - 1);
  return ((chksum_val == chksum_calc) ? 1 : 0);
}

int8_t 
ipmi_comp_test (fiid_obj_t obj_cmd)
{
#if defined (IPMI_SYSLOG)
  uint64_t cmd;
#endif /* IPMI_SYSLOG */
  uint64_t comp_code;
  int32_t len;
  int8_t rv;

  if (!fiid_obj_valid(obj_cmd))
    {
      errno = EINVAL;
      return (-1);
    }

#if defined (IPMI_SYSLOG)
  if ((rv = fiid_obj_field_lookup (obj_cmd, (uint8_t *)"cmd")) < 0)
    return (-1);

  if (!rv)
    {
      errno = EINVAL;
      return (-1);
    }
#endif /* IPMI_SYSLOG */

  if ((rv = fiid_obj_field_lookup (obj_cmd, (uint8_t *)"comp_code")) < 0)
    return (-1);

  if (!rv)
    {
      errno = EINVAL;
      return (-1);
    }

  if ((len = fiid_obj_field_len (obj_cmd, (uint8_t *)"comp_code")) < 0)
    return (-1);

  if (!len)
    {
      errno = EINVAL;
      return (-1);
    }

#if defined (IPMI_SYSLOG)
  if (fiid_obj_get(obj_cmd, (uint8_t *)"cmd", &cmd) < 0)
    return (-1);
#endif /* IPMI_SYSLOG */

  if (fiid_obj_get(obj_cmd, (uint8_t *)"comp_code", &comp_code) < 0)
    return (-1);

  if (comp_code != IPMI_COMP_CODE_COMMAND_SUCCESS)
    {
#if defined (IPMI_SYSLOG)
      char errstr[IPMI_ERR_STR_MAX_LEN], _str[IPMI_ERR_STR_MAX_LEN];
      ipmi_strerror_cmd_r (obj_cmd, _str, IPMI_ERR_STR_MAX_LEN);
      sprintf (errstr, "cmd[%llX].comp_code[%llX]: %s",
               cmd, comp_code, _str);
      syslog (LOG_MAKEPRI (LOG_FAC (LOG_LOCAL1), LOG_ERR), errstr);
#endif /* IPMI_SYSLOG */
      errno = EIO;
      return (0);
    }
  return (1);
}

int
ipmi_open_free_udp_port (void)
{
  int sockfd;
  int sockname_len;
  struct sockaddr_in sockname;
  int free_port=1025;
  int err;
  extern int errno;

  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    return (-1);

  for (; free_port < 65535; free_port++)
    {
      /* Instead of probing if the current (may be the client side)
      system has IPMI LAN support too, it is easier to avoid these two
      RMCP reserved ports. -- Anand Babu*/
      if ((free_port == RMCP_AUX_BUS_SHUNT) || 
	  (free_port == RMCP_SECURE_AUX_BUS))
	continue;

      memset (&sockname, 0, sizeof (struct sockaddr_in));
      sockname.sin_family = AF_INET;
      sockname.sin_port   = htons (free_port);
      sockname.sin_addr.s_addr = htonl (INADDR_ANY);
      sockname_len = sizeof (struct sockaddr_in);
      
      if ((err = bind (sockfd, (struct sockaddr *) &sockname, sockname_len)) == 0)
	return sockfd;
      else
	{
	  if (errno == EADDRINUSE)
	    continue;
	  else
	    return (-1);
	}
    }
  close (sockfd);
  errno = EBUSY;
  return (-1);
}
