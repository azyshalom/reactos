/* $Id: $
 *
 * smapiquery.c - SM_API_QUERY_INFORMATION
 *
 * Reactos Session Manager
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"

#define NDEBUG
#include <debug.h>


/**********************************************************************
 * SmQryInfo/1							API
 */
SMAPI(SmQryInfo)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("SM: %s called\n", __FUNCTION__);
	
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return Status;
}


/* EOF */
