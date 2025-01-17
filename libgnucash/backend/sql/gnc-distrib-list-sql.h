/*
 * gnc-distrib-list-sql.h -- distribution list sql backend
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

/** @file gnc-distrib-list-sql.h
 *  @brief load and save accounts data to SQL
 *  @author Copyright (c) 2022 Ralf Zerres <ralf.zerres@mail.de>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring distribution list data to/from an SQL database
 */

#ifndef GNC_DISTRIBLIST_SQL_H
#define GNC_DISTRIBLIST_SQL_H

#include "gnc-sql-object-backend.hpp"

class GncSqlDistribListBackend : public GncSqlObjectBackend
{
public:
    GncSqlDistribListBackend();
    void load_all(GncSqlBackend*) override;
    void create_tables(GncSqlBackend*) override;
    bool write(GncSqlBackend*) override;
};

#endif /* GNC_DISTRIBLIST_SQL_H */
