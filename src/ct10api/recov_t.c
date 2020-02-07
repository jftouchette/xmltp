/****************************************************************************
 * recov_t.c
 *
 *  Copyright (c) 1997-2001 Jocelyn Blanchet
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Library General Public License text).
 *  -------------------------------------------------------------------------
 *
 * $Source: /ext_hd/cvs/ct10api/recov_t.c,v $
 * $Header: /ext_hd/cvs/ct10api/recov_t.c,v 1.2 2004/10/13 18:23:27 blanchjj Exp $
 *
 *  Master application level recovery table (for ct10api, ct10reco).
 *
 *
 * 1997-08-15,jbl:	Initial creation.
 */

CTAPRECO_RECOV_TABLE_T tab_ctapreco[] = {

#include "recov_t2.c"

#include "recov_t3.c"

}; /* End of tab_ctapreco */

#define NBROW_TAB_CTAPRECO  \
	((sizeof(tab_ctapreco) / sizeof(CTAPRECO_RECOV_TABLE_T)) )

