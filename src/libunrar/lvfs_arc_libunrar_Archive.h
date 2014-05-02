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

#ifndef LVFS_ARC_LIBUNRAR_ARCHIVE_H_
#define LVFS_ARC_LIBUNRAR_ARCHIVE_H_

#include <cstdio>
#include <lvfs/IType>
#include <lvfs/IFile>
#include <lvfs/IFsFile>
#include <lvfs/IDirectory>
#include <lvfs-arc/IArchive>


namespace LVFS {
namespace Arc {
namespace LibUnrar {

class PLATFORM_MAKE_PRIVATE Archive : public Implements<IFile, IDirectory, IEntry, IFsFile, IArchive>
{
public:
    Archive(const Interface::Holder &file);
    virtual ~Archive();

    /* IFile */

    virtual bool open();
    virtual size_t read(void *buffer, size_t size);
    virtual size_t write(const void *buffer, size_t size);
    virtual bool seek(long offset, Whence whence);
    virtual bool flush();
    virtual void close();

    virtual uint64_t size() const;
    virtual size_t position() const;

    /* IDirectory */

    virtual const_iterator begin() const;
    virtual const_iterator end() const;
    virtual Interface::Holder entry(const char *name) const;

    virtual bool rename(const Interface::Holder &file, const char *name);
    virtual bool remove(const Interface::Holder &file);

    /* IEntry */

    virtual const char *title() const;
    virtual const char *location() const;
    virtual const IType *type() const;

    /* IFsFile */

    virtual time_t cTime() const;
    virtual time_t mTime() const;
    virtual time_t aTime() const;

    virtual int permissions() const;
    virtual bool setPermissions(int value);

    /* IArchive */

    virtual const char *password() const;
    virtual void setPassword(const char *value);

    /* COMMON */

    virtual const Error &lastError() const;

private:
    Interface::Holder m_file;
    char *m_password;
    mutable Error m_error;
    mutable const Error *m_lastError;
};

}}}

#endif /* LVFS_ARC_LIBUNRAR_ARCHIVE_H_ */
