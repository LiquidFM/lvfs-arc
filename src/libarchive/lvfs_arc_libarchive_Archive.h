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

#ifndef LVFS_ARC_LIBARCHIVE_ARCHIVE_H_
#define LVFS_ARC_LIBARCHIVE_ARCHIVE_H_

#include "../lvfs_arc_Archive.h"


namespace LVFS {
namespace Arc {
namespace LibArchive {

class PLATFORM_MAKE_PRIVATE Archive : public Arc::Archive
{
public:
    Archive(const Interface::Holder &file);
    virtual ~Archive();

public: /* IDirectory */
    virtual const_iterator begin() const;
};

}}}

#endif /* LVFS_ARC_LIBARCHIVE_ARCHIVE_H_ */
