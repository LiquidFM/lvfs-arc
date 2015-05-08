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

#ifndef LVFS_ARC_ARCHIVE_H_
#define LVFS_ARC_ARCHIVE_H_

#include <efc/Map>
#include <efc/String>
#include <efc/Holder>
#include <lvfs/IDirectory>
#include <lvfs-arc/IArchive>


namespace LVFS {
namespace Arc {

class PLATFORM_MAKE_PRIVATE Archive : public ExtendsBy<IDirectory, IArchive>
{
public:
    typedef EFC::Map<EFC::String, Interface::Holder> Entries;

public:
    class PLATFORM_MAKE_PRIVATE Reader;
    typedef ::EFC::Holder<Reader> ReaderHolder;

public:
    Archive(const Interface::Holder &file);
    virtual ~Archive();

public: /* IDirectory */
    virtual const_iterator end() const;

    virtual bool exists(const char *name) const;
    virtual Interface::Holder entry(const char *name, const IType *type = NULL, bool create = false);

    virtual bool copy(const Progress &callback, const Interface::Holder &file, bool move = false);
    virtual bool rename(const Interface::Holder &file, const char *name);
    virtual bool remove(const Interface::Holder &file);

public: /* IArchive */
    virtual const char *password() const;
    virtual void setPassword(const char *value);

public: /* COMMON */
    virtual const Error &lastError() const;

protected:
    const Entries &entries() const { return m_entries; }
    bool process(ReaderHolder &reader);

private:
    char *m_password;
    Entries m_entries;
    mutable Error m_error;
    mutable const Error *m_lastError;
};


class PLATFORM_MAKE_PRIVATE Archive::Reader : public ReaderHolder::Data
{
public:
    typedef ReaderHolder Holder;

public:
    Reader(const Interface::Holder &file, const char *password);
    virtual ~Reader();

    virtual bool isOpen() const = 0;
    virtual bool open() = 0;
    virtual size_t read(void *buffer, size_t size) = 0;
    virtual void close() = 0;
    virtual bool next() = 0;

    virtual const char *archive_entry_pathname() const = 0;
    virtual time_t archive_entry_ctime() const = 0;
    virtual time_t archive_entry_mtime() const = 0;
    virtual time_t archive_entry_atime() const = 0;
    virtual mode_t archive_entry_perm() const = 0;
    virtual int64_t archive_entry_size() const = 0;

protected:
    inline const Interface::Holder &file() const { return m_file; }

    inline const char *password() const { return m_password; }
    void setPassword(const char *value);

private:
    Interface::Holder m_file;
    char *m_password;
};

}}

#endif /* LVFS_ARC_ARCHIVE_H_ */
