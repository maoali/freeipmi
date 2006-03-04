/*****************************************************************************\
 *  $Id: md2.c,v 1.2.2.2 2006-03-04 03:40:18 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2003 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-155698
 *
 *  This file is part of Ipmipower, a remote power control utility.
 *  For details, see http://www.llnl.gov/linux/.
 *
 *  Ipmipower is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmipower is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmipower; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02110-1301  USA.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif
#include <errno.h>

#include "md2.h"

static char padding[16][16] = 
  {
    {0x01},
    {0x02,0x02},
    {0x03,0x03,0x03},
    {0x04,0x04,0x04,0x04},
    {0x05,0x05,0x05,0x05,0x05},
    {0x06,0x06,0x06,0x06,0x06,0x06},
    {0x07,0x07,0x07,0x07,0x07,0x07,0x07},
    {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08},
    {0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09},
    {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A},
    {0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B},
    {0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C},
    {0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D},
    {0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E},
    {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10}
  };

static unsigned char S[256] = 
  {
    0x29, 0x2E, 0x43, 0xC9, 0xA2, 0xD8, 0x7C, 0x01, 
    0x3D, 0x36, 0x54, 0xA1, 0xEC, 0xF0, 0x06, 0x13, 
    0x62, 0xA7, 0x05, 0xF3, 0xC0, 0xC7, 0x73, 0x8C, 
    0x98, 0x93, 0x2B, 0xD9, 0xBC, 0x4C, 0x82, 0xCA, 
    0x1E, 0x9B, 0x57, 0x3C, 0xFD, 0xD4, 0xE0, 0x16, 
    0x67, 0x42, 0x6F, 0x18, 0x8A, 0x17, 0xE5, 0x12, 
    0xBE, 0x4E, 0xC4, 0xD6, 0xDA, 0x9E, 0xDE, 0x49, 
    0xA0, 0xFB, 0xF5, 0x8E, 0xBB, 0x2F, 0xEE, 0x7A, 
    0xA9, 0x68, 0x79, 0x91, 0x15, 0xB2, 0x07, 0x3F, 
    0x94, 0xC2, 0x10, 0x89, 0x0B, 0x22, 0x5F, 0x21, 
    0x80, 0x7F, 0x5D, 0x9A, 0x5A, 0x90, 0x32, 0x27, 
    0x35, 0x3E, 0xCC, 0xE7, 0xBF, 0xF7, 0x97, 0x03, 
    0xFF, 0x19, 0x30, 0xB3, 0x48, 0xA5, 0xB5, 0xD1, 
    0xD7, 0x5E, 0x92, 0x2A, 0xAC, 0x56, 0xAA, 0xC6, 
    0x4F, 0xB8, 0x38, 0xD2, 0x96, 0xA4, 0x7D, 0xB6, 
    0x76, 0xFC, 0x6B, 0xE2, 0x9C, 0x74, 0x04, 0xF1, 
    0x45, 0x9D, 0x70, 0x59, 0x64, 0x71, 0x87, 0x20, 
    0x86, 0x5B, 0xCF, 0x65, 0xE6, 0x2D, 0xA8, 0x02, 
    0x1B, 0x60, 0x25, 0xAD, 0xAE, 0xB0, 0xB9, 0xF6, 
    0x1C, 0x46, 0x61, 0x69, 0x34, 0x40, 0x7E, 0x0F, 
    0x55, 0x47, 0xA3, 0x23, 0xDD, 0x51, 0xAF, 0x3A, 
    0xC3, 0x5C, 0xF9, 0xCE, 0xBA, 0xC5, 0xEA, 0x26, 
    0x2C, 0x53, 0x0D, 0x6E, 0x85, 0x28, 0x84, 0x09, 
    0xD3, 0xDF, 0xCD, 0xF4, 0x41, 0x81, 0x4D, 0x52, 
    0x6A, 0xDC, 0x37, 0xC8, 0x6C, 0xC1, 0xAB, 0xFA, 
    0x24, 0xE1, 0x7B, 0x08, 0x0C, 0xBD, 0xB1, 0x4A, 
    0x78, 0x88, 0x95, 0x8B, 0xE3, 0x63, 0xE8, 0x6D, 
    0xE9, 0xCB, 0xD5, 0xFE, 0x3B, 0x00, 0x1D, 0x39, 
    0xF2, 0xEF, 0xB7, 0x0E, 0x66, 0x58, 0xD0, 0xE4, 
    0xA6, 0x77, 0x72, 0xF8, 0xEB, 0x75, 0x4B, 0x0A, 
    0x31, 0x44, 0x50, 0xB4, 0x8F, 0xED, 0x1F, 0x1A, 
    0xDB, 0x99, 0x8D, 0x33, 0x9F, 0x11, 0x83, 0x14
  };

#define L          ctx->l  
#define X          ctx->x 
#define C          ctx->c
#define M          ctx->m
#define Mlen       ctx->mlen
#define MD2_MAGIC  0xf00fd00d 

int 
md2_init(md2_t *ctx) 
{

  if (ctx == NULL) 
    {
      errno = EINVAL;
      return -1;
    }

  ctx->magic = MD2_MAGIC;

  L = 0;
  Mlen = 0;
  memset(X, '\0', MD2_BUFFER_LENGTH);
  memset(C, '\0', MD2_CHKSUM_LENGTH);
  memset(M, '\0', MD2_BLOCK_LENGTH);

  return 0;
}

static void 
_md2_update_digest_and_checksum(md2_t *ctx) 
{
  int j, k;
  uint8_t c, t;

  /* Update X */

  for (j = 0; j < MD2_BLOCK_LENGTH; j++) 
    {
      X[16+j] = M[j];
      X[32+j] = (X[16+j] ^ X[j]);
    }
  
  t = 0;

  for (j = 0; j < MD2_ROUNDS_LENGTH; j++) 
    {
      for (k = 0; k < MD2_BUFFER_LENGTH; k++) 
        {
          t = X[k] = (X[k] ^ S[t]);
        }
      t = (t + j) % 256;
    }

  /* Update C and L */
  
  /* achu: Note that there is a typo in the RFC 1319 specification.
   * In section 3.2, the line:
   *
   * Set C[j] to S[c xor L]
   *
   * should read:
   *
   * Set C[j] to C[j] xor S[c xor L].
   */
  
  for (j = 0; j < MD2_BLOCK_LENGTH; j++) 
    {
      c = M[j];
      C[j] = C[j] ^ S[c ^ L];
      L = C[j];
    }
}

int 
md2_update_data(md2_t *ctx, uint8_t *buf, unsigned int buflen) 
{

  if (ctx == NULL || ctx->magic != MD2_MAGIC || buf == NULL) 
    {
      errno = EINVAL;
      return -1;
    }

  if (buflen == 0)
    return 0;

  if ((Mlen + buflen) >= MD2_BLOCK_LENGTH) 
    {
      unsigned int bufcount;
      
      bufcount = (MD2_BLOCK_LENGTH - Mlen);
      memcpy(M + Mlen, buf, bufcount);
      _md2_update_digest_and_checksum(ctx);
    
      while ((buflen - bufcount) >= MD2_BLOCK_LENGTH) 
        {
          memcpy(M, buf + bufcount, MD2_BLOCK_LENGTH);
          bufcount += MD2_BLOCK_LENGTH;
          _md2_update_digest_and_checksum(ctx);
        }
      
      Mlen = buflen - bufcount;
      if (Mlen > 0)
        memcpy(M, buf + bufcount, Mlen);
    }
  else 
    {
      /* Not enough data to update X and C, just copy in data */ 
      memcpy(M + Mlen, buf, buflen); 
      Mlen += buflen;
    }

  return buflen;
}

static void 
_md2_append_padding_and_checksum(md2_t *ctx) 
{
  unsigned int padlen;
  int padindex;

  padlen = MD2_PADDING_LENGTH - Mlen;
  padindex = padlen - 1;

  md2_update_data(ctx, (uint8_t *)padding[padindex], (int)padlen);
  
  md2_update_data(ctx, C, MD2_CHKSUM_LENGTH);
}

int 
md2_finish(md2_t *ctx, uint8_t *digest, unsigned int digestlen) 
{
  
  if (ctx == NULL || ctx->magic != MD2_MAGIC 
      || digest == NULL || digestlen < MD2_DIGEST_LENGTH) 
    {
      errno = EINVAL;
      return -1;
    }
  
  _md2_append_padding_and_checksum(ctx);
  memcpy(digest, X, MD2_DIGEST_LENGTH);
  
  ctx->magic = ~MD2_MAGIC;
  return MD2_DIGEST_LENGTH;
}

