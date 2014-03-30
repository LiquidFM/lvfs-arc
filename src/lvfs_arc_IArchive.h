/**
 * This file is part of lvfs-arc.
 *
 * Copyright (C) 2011-2014 Dmitriy Vilkov, <dav.daemon@gmail.com>
 *
 * lvfs-arc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lvfs-arc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lvfs-arc. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LVFS_ARC_IARCHIVE_H_
#define LVFS_ARC_IARCHIVE_H_

#include <lvfs/Interface>


namespace LVFS {
namespace Arc {

class PLATFORM_MAKE_PUBLIC IArchive
{
    DECLARE_INTERFACE(LVFS::Arc::IArchive)

public:
    virtual ~IArchive();

    virtual const char *password() const = 0;
    virtual void setPassword(const char *value) = 0;
};

}}

#endif /* LVFS_ARC_IARCHIVE_H_ */
