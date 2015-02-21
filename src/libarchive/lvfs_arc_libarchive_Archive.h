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

#include <lvfs/IDirectory>
#include <lvfs-arc/IArchive>


namespace LVFS {
namespace Arc {
namespace LibArchive {

class PLATFORM_MAKE_PRIVATE Archive : public ExtendsBy<IDirectory, IArchive>
{
public:
    Archive(const Interface::Holder &file);
    virtual ~Archive();

    /* IDirectory */

    virtual const_iterator begin() const;
    virtual const_iterator end() const;

    virtual bool exists(const char *name) const;
    virtual Interface::Holder entry(const char *name, const IType *type = NULL, bool create = false);

    virtual bool copy(const Progress &callback, const Interface::Holder &file, bool move = false);
    virtual bool rename(const Interface::Holder &file, const char *name);
    virtual bool remove(const Interface::Holder &file);

    /* IArchive */

    virtual const char *password() const;
    virtual void setPassword(const char *value);

    /* COMMON */

    virtual const Error &lastError() const;

private:
    char *m_password;
    mutable Error m_error;
    mutable const Error *m_lastError;
};

}}}

#endif /* LVFS_ARC_LIBARCHIVE_ARCHIVE_H_ */
