/********************************************************************\
 * gncDistributionListP.h -- the Gnucash distributon list           *
 *                           private interface                      *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/*
 * Copyright (C) 2022 Ralf Zerres
 * Author: Ralf Zerres <ralf.zerres@mail.de>
*/

#ifndef GNC_DISTRIBUTIONLISTP_H_
#define GNC_DISTRIBUTIONLISTP_H_

#include "gncDistributionList.h"

gboolean gncDistribListRegister (void);

#define gncDistribListSetGUID(D,G) qof_instance_set_guid(QOF_INSTANCE(D),(G))

void gncDistribListSetParent (GncDistributionList *distriblist, GncDistributionList *parent);
void gncDistribListSetChild (GncDistributionList *distriblist, GncDistributionList *child);
void gncDistribListSetRefcount (GncDistributionList *distriblist, gint64 refcount);
void gncDistribListMakeInvisible (GncDistributionList *distriblist);

gboolean gncDistribListGetInvisible (const GncDistributionList *distriblist);

#endif /* GNC_DISTRIBUTIONLISTP_H_ */
