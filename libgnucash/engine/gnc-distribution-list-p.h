/********************************************************************\
 * gnc-distribution-list-p.hpp -- the Gnucash distributon list       *
 *                                 private interface                 *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License as    *
 * published by the Free Software Foundation; either version 2 of    *
 * the License, or (at your option) any later version.               *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License for more details.                      *
 *                                                                   *
 * You should have received a copy of the GNU General Public License *
 * along with this program; if not, contact:                         *
 *                                                                   *
 * Free Software Foundation           Voice:  +1-617-542-5942        *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652        *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                    *
 *                                                                   *
\*********************************************************************/

/** @addtogroup Business
    @{ */
/** @addtogroup DistributionList
    @{ */
/** @file gnc-distribution-list.hpp
 *  @author Copyright (C) 2025 Ralf Zerres <ralf.zerres@mail.de>
 *  @brief Distribution list handling public routines (C++ api)
 *
 * This is the *private* header for the DistributionList structure.
 * No one outside of the engine should ever include this file.
 *
 * This header includes prototypes for "dangerous" functions.
 * Invoking any of these functions potentially leave the account
 * in an inconsistent state.  If they are not used in the proper
 * setting, they can leave the account structures in an inconsistent
 * state.  Thus, these methods should never be used outside of
 * the engine, which is why they are "hidden" here.
 *
 */


#ifndef GNC_DISTRIBUTIONLISTP_P_H
#define GNC_DISTRIBUTIONLISTP_P_H

#include <gnc-distribution-list.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register distribution lists with the engine */
gboolean gncDistribListRegister (void);

void gncDistribListSetParent (GncDistributionList *distriblist, GncDistributionList *parent);
void gncDistribListSetChild (GncDistributionList *distriblist, GncDistributionList *child);
void gncDistribListSetRefcount (GncDistributionList *distriblist, gint64 refcount);
void gncDistribListMakeInvisible (GncDistributionList *distriblist);

gboolean gncDistribListGetInvisible (const GncDistributionList *distriblist);

/* Set the distibution list's GncGUID. This should only be done when reading
 * a distribution list from a datafile, or some other external source. Never
 * call this on an existing distribution list! */
//void xaccAccountSetGUID (Account *account, const GncGUID *guid);
#define gncDistribListSetGUID(D,G) qof_instance_set_guid(QOF_INSTANCE(D),(G))


#ifdef __cplusplus
}
#endif

#endif /* GNC_DISTRIBUTIONLISTP_P_H */
