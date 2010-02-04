/*
  Copyright (C) 2003-2010 FreeIPMI Core Team

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
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifdef STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <errno.h>

#include "freeipmi/interpret/ipmi-interpret.h"

#include "ipmi-interpret-defs.h"
#include "ipmi-interpret-trace.h"
#include "ipmi-interpret-util.h"

#include "freeipmi-portability.h"

void
interpret_set_interpret_errnum_by_errno (ipmi_interpret_ctx_t ctx, int __errno)
{
  if (!ctx || ctx->magic != IPMI_INTERPRET_CTX_MAGIC)
    return;

  if (__errno == 0)
    ctx->errnum = IPMI_INTERPRET_ERR_SUCCESS;
  else if (__errno == ENOMEM)
    ctx->errnum = IPMI_INTERPRET_ERR_OUT_OF_MEMORY;
  else
    ctx->errnum = IPMI_INTERPRET_ERR_INTERNAL_ERROR;
}
