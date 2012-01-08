/*****************************************************************************\
 *  $Id: ipmipower_ping.c,v 1.54 2010-02-08 22:02:31 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2007-2012 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2003-2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-155698
 *
 *  This file is part of Ipmipower, a remote power control utility.
 *  For details, see http://www.llnl.gov/linux/.
 *
 *  Ipmipower is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmipower is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmipower.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else  /* !TIME_WITH_SYS_TIME */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif  /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */
#include <errno.h>

#include "ipmipower_ping.h"
#include "ipmipower_error.h"
#include "ipmipower_util.h"

#include "freeipmi-portability.h"
#include "cbuf.h"
#include "debug-util.h"
#include "timeval.h"

extern struct ipmipower_arguments cmd_args;
extern struct ipmipower_connection *ics;
extern unsigned int ics_len;

/* next_ping_sends_time, when the next round of pings should be sent */
static struct timeval next_ping_sends_time;

/* force discovery sweep when user reconfigures hostnames */
static int force_discovery_sweep;

void
ipmipower_ping_force_discovery_sweep ()
{
  force_discovery_sweep = 1;
}

void
ipmipower_ping_process_pings (int *timeout)
{
  int i, send_pings_flag = 0;
  struct timeval cur_time, result;
  unsigned int ms_time;

  assert (timeout);

  if (!cmd_args.common.hostname)
    return;

  if (!cmd_args.ping_interval)
    return;

  if (gettimeofday (&cur_time, NULL) < 0)
    {
      IPMIPOWER_ERROR (("gettimeofday: %s", strerror (errno)));
      exit (1);
    }

  if (timeval_gt (&cur_time, &next_ping_sends_time) || force_discovery_sweep)
    {
      force_discovery_sweep = 0;
      timeval_add_ms (&cur_time, cmd_args.ping_interval, &next_ping_sends_time);
      send_pings_flag++;
    }

  for (i = 0; i < ics_len; i++)
    {
      uint8_t buf[IPMIPOWER_PACKET_BUFLEN];
      int ret, len;

      if (send_pings_flag)
        {
          fiid_obj_t rmcp_hdr = NULL;
          fiid_obj_t rmcp_ping = NULL;
          int dropped = 0;
          
          memset (buf, '\0', IPMIPOWER_PACKET_BUFLEN);

          /* deal with packet heuristics */
          if (cmd_args.ping_packet_count && cmd_args.ping_percent)
            {
              if (ics[i].ping_packet_count_send == cmd_args.ping_packet_count)
                {
                  if ((((double)(ics[i].ping_packet_count_send - ics[i].ping_packet_count_recv))/ics[i].ping_packet_count_send) > ((double)cmd_args.ping_percent/100))
                    ics[i].link_state = LINK_BAD;
                  else
                    ics[i].link_state = LINK_GOOD;

                  ics[i].ping_packet_count_send = 0;
                  ics[i].ping_packet_count_recv = 0;
                }
            }

          if (cmd_args.ping_consec_count)
            {
              if (!ics[i].ping_last_packet_recv_flag)
                ics[i].ping_consec_count = 0;
              ics[i].ping_last_packet_recv_flag = 0;
            }

          /* must increment count before setting message tag, so we
           * can check sequence number correctly later on
           */
          ics[i].ping_sequence_number_counter++;

          if (!(rmcp_hdr = fiid_obj_create (tmpl_rmcp_hdr)))
            {
              IPMIPOWER_ERROR (("fiid_obj_create: %s", strerror (errno)));
              exit (1);
            }

          if (!(rmcp_ping = fiid_obj_create (tmpl_cmd_asf_presence_ping)))
            {
              IPMIPOWER_ERROR (("fiid_obj_create: %s", strerror (errno)));
              exit (1);
            }

          if (fill_rmcp_hdr_asf (rmcp_hdr) < 0)
            {
              IPMIPOWER_ERROR (("fill_rmcp_hdr_asf: %s", strerror (errno)));
              exit (1);
            }

          if (fill_cmd_asf_presence_ping ((ics[i].ping_sequence_number_counter %
                                           (RMCP_ASF_MESSAGE_TAG_MAX + 1)),
                                          rmcp_ping) < 0)
            {
              IPMIPOWER_ERROR (("fill_cmd_asf_presence_ping: %s", strerror (errno)));
              exit (1);
            }

          if ((len = assemble_rmcp_pkt (rmcp_hdr,
                                        rmcp_ping,
                                        buf,
                                        IPMIPOWER_PACKET_BUFLEN,
					IPMI_INTERFACE_FLAGS_DEFAULT)) < 0)
            {
              IPMIPOWER_ERROR (("assemble_rmcp_pkt: %s", strerror (errno)));
              exit (1);
            }

#ifndef NDEBUG
          if (cmd_args.rmcpdump)
            {
              char hdrbuf[DEBUG_UTIL_HDR_BUFLEN];

              debug_hdr_str (DEBUG_UTIL_TYPE_NONE,
                             DEBUG_UTIL_DIRECTION_NONE,
			     DEBUG_UTIL_FLAGS_DEFAULT,
                             DEBUG_UTIL_RMCPPING_STR,
                             hdrbuf,
                             DEBUG_UTIL_HDR_BUFLEN);

              if (ipmi_dump_rmcp_packet (STDERR_FILENO,
                                         ics[i].hostname,
                                         hdrbuf,
                                         NULL,
                                         buf,
                                         len,
                                         tmpl_cmd_asf_presence_ping) < 0)
                IPMIPOWER_DEBUG (("ipmi_dump_rmcp_packet: %s", strerror (errno)));
            }
#endif /* NDEBUG */

          if ((ret = cbuf_write (ics[i].ping_out, buf, len, &dropped)) < 0)
            {
              IPMIPOWER_ERROR (("cbuf_write: %s", strerror (errno)));
              exit (1);
            }

          if (ret != len)
            {
              IPMIPOWER_ERROR (("cbuf_write: incorrect bytes written %d", ret));
              exit (1);
            }

          if (dropped)
            IPMIPOWER_DEBUG (("cbuf_write: dropped %d bytes", dropped));

          ics[i].last_ping_send.tv_sec = cur_time.tv_sec;
          ics[i].last_ping_send.tv_usec = cur_time.tv_usec;

          if (cmd_args.ping_packet_count && cmd_args.ping_percent)
            ics[i].ping_packet_count_send++;

          fiid_obj_destroy (rmcp_hdr);
          fiid_obj_destroy (rmcp_ping);
        }

      /* Did we receive something? */
      memset (buf, '\0', IPMIPOWER_PACKET_BUFLEN);
      len = ipmipower_cbuf_peek_and_drop (ics[i].ping_in, buf, IPMIPOWER_PACKET_BUFLEN);
      if (len > 0)
        {
          fiid_obj_t rmcp_hdr = NULL;
          fiid_obj_t rmcp_pong = NULL;
          uint8_t message_type, ipmi_supported;
          uint64_t val;

          if (!(rmcp_hdr = fiid_obj_create (tmpl_rmcp_hdr)))
            {
              IPMIPOWER_ERROR (("fiid_obj_create: %s", strerror (errno)));
              exit (1);
            }

          if (!(rmcp_pong = fiid_obj_create (tmpl_cmd_asf_presence_pong)))
            {
              IPMIPOWER_ERROR (("fiid_obj_create: %s", strerror (errno)));
              exit (1);
            }

#ifndef NDEBUG
          if (cmd_args.rmcpdump)
            {
              char hdrbuf[DEBUG_UTIL_HDR_BUFLEN];

              debug_hdr_str (DEBUG_UTIL_TYPE_NONE,
                             DEBUG_UTIL_DIRECTION_NONE,
			     DEBUG_UTIL_FLAGS_DEFAULT,
                             DEBUG_UTIL_RMCPPING_STR,
                             hdrbuf,
                             DEBUG_UTIL_HDR_BUFLEN);

              if (ipmi_dump_rmcp_packet (STDERR_FILENO,
                                         ics[i].hostname,
                                         hdrbuf,
                                         NULL,
                                         buf,
                                         len,
                                         tmpl_cmd_asf_presence_pong) < 0)
                IPMIPOWER_DEBUG (("ipmi_dump_rmcp_packet: %s", strerror (errno)));
            }
#endif /* NDEBUG */

          if ((ret = unassemble_rmcp_pkt (buf,
                                          len,
                                          rmcp_hdr,
                                          rmcp_pong,
					  IPMI_INTERFACE_FLAGS_DEFAULT)) < 0)
            {
              IPMIPOWER_ERROR (("unassemble_rmcp_pkt: %s", strerror (errno)));
              exit (1);
            }

          if (ret)
            {
              /* achu: check for ipmi_support and pong type, but don't
               * check for message tag.  On occassion, I have witnessed
               * BMCs send message tags "out of sync".  For example, you
               * send 8, BMC returns 7.  You send 9, BMC returns 8.  We
               * really don't care if the BMC is out of sync.  We just
               * need to make sure we get something back from the BMC to
               * ensure the machine is still there.
               */
              
              if (FIID_OBJ_GET (rmcp_pong,
                                "message_type",
                                &val) < 0)
                {
                  IPMIPOWER_ERROR (("FIID_OBJ_GET: 'message_type': %s",
                                    fiid_obj_errormsg (rmcp_pong)));
                  exit (1);
                }
              message_type = val;
              
              if (FIID_OBJ_GET (rmcp_pong,
                                "supported_entities.ipmi_supported",
                                &val) < 0)
                {
                  IPMIPOWER_ERROR (("FIID_OBJ_GET: 'supported_entities.ipmi_supported': %s",
                                    fiid_obj_errormsg (rmcp_pong)));
                  exit (1);
                }
              ipmi_supported = val;
              
              if (message_type == RMCP_ASF_MESSAGE_TYPE_PRESENCE_PONG && ipmi_supported)
                {
                  if (cmd_args.ping_packet_count && cmd_args.ping_percent)
                    ics[i].ping_packet_count_recv++;
                  
                  if (cmd_args.ping_consec_count)
                    {
                      /* Don't increment twice, its possible a previous pong
                       * response was late, and we quickly receive two
                       * pong responses
                       */
                      if (!ics[i].ping_last_packet_recv_flag)
                        ics[i].ping_consec_count++;
                      
                      ics[i].ping_last_packet_recv_flag++;
                    }
                  
                  if (cmd_args.ping_packet_count && cmd_args.ping_percent)
                    {
                      if (ics[i].link_state == LINK_GOOD)
                        ics[i].discover_state = STATE_DISCOVERED;
                      else
                        {
                          if (cmd_args.ping_consec_count
                              && ics[i].ping_consec_count >= cmd_args.ping_consec_count)
                            ics[i].discover_state = STATE_DISCOVERED;
                          else
                            ics[i].discover_state = STATE_BADCONNECTION;
                        }
                    }
                  else
                    {
                      ics[i].discover_state = STATE_DISCOVERED;
                    }
                  ics[i].last_ping_recv.tv_sec = cur_time.tv_sec;
                  ics[i].last_ping_recv.tv_usec = cur_time.tv_usec;
                  
                  fiid_obj_destroy (rmcp_hdr);
                  fiid_obj_destroy (rmcp_pong);
                }
            }
        }

      /* Is the node gone?? */
      timeval_sub (&cur_time, &ics[i].last_ping_recv, &result);
      timeval_millisecond_calc (&result, &ms_time);
      if (ms_time >= cmd_args.ping_timeout)
        ics[i].discover_state = STATE_UNDISCOVERED;
    }

  timeval_sub (&next_ping_sends_time, &cur_time, &result);
  timeval_millisecond_calc (&result, &ms_time);
  *timeout = ms_time;
}