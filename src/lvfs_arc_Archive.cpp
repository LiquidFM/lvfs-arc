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

#include <lvfs/Module>
#include <lvfs/IProperties>
#include <brolly/assert.h>

#include <cstdlib>
#include <cstring>
#include <archive.h>
#include <archive_entry.h>

#include <errno.h>
#include "lvfs_arc_Archive.h"


namespace LVFS {
namespace Arc {
namespace {


    class ArchiveEntryFile : public Implements<IStream>
    {
    public:
        ArchiveEntryFile(const Archive::Reader::Holder &reader) :
            m_reader(reader)
        {
            ASSERT(m_reader.isValid());
        }

    public: /* IStream */
        virtual size_t read(void *buffer, size_t size) { return m_reader->read(buffer, size); }
        virtual size_t write(const void *buffer, size_t size) { m_error = Error(EROFS); return false; }
        virtual bool advise(off_t offset, off_t len, Advise advise) { m_error = Error(EROFS); return false; }
        virtual bool seek(long offset, Whence whence) { m_error = Error(EROFS); return false; }
        virtual bool flush() { m_error = Error(EROFS); return false; }

        virtual const Error &lastError() const { return m_error; }

    private:
        mutable Error m_error;
        Archive::Reader::Holder m_reader;
    };


    class ArchiveEntry : public Implements<IEntry, IProperties>
    {
    public:
        ArchiveEntry(const Archive::Reader::Holder &reader) :
            m_reader(reader),
            m_path(::strdup(m_reader->archive_entry_pathname())),
            m_title(::strrchr(m_path, '/')),
            m_cTime(m_reader->archive_entry_ctime()),
            m_mTime(m_reader->archive_entry_mtime()),
            m_aTime(m_reader->archive_entry_atime()),
            m_perm(m_reader->archive_entry_perm()),
            m_size(m_reader->archive_entry_size())
        {
            if (m_title != NULL)
                ++m_title;
            else
                m_title = m_path;
        }

        virtual ~ArchiveEntry()
        {
            ::free(m_path);
        }

        void initType()
        {
            m_type = Module::desktop().typeOfFile(this);
            ASSERT(m_type.isValid());
        }

    public: /* IEntry */
        virtual const char *title() const { return m_title; }
        virtual const char *schema() const { return "file"; }
        virtual const char *location() const { return m_path; }
        virtual const IType *type() const { return m_type; }
        virtual Interface::Holder open(IStream::Mode mode) const
        {
            if (mode == IStream::Read)
                if (m_reader->isOpen() && ::strcmp(m_path, m_reader->archive_entry_pathname()) == 0)
                    return Interface::Holder(new (std::nothrow) ArchiveEntryFile(m_reader));
                else
                {
                    m_reader->close();

                    if (m_reader->open())
                        while (m_reader->next())
                            if (::strcmp(m_path, m_reader->archive_entry_pathname()) == 0)
                                return Interface::Holder(new (std::nothrow) ArchiveEntryFile(m_reader));
                }

            return Interface::Holder();
        }

    public: /* IProperties */
        virtual off64_t size() const { return m_size; }
        virtual time_t cTime() const { return m_cTime; }
        virtual time_t mTime() const { return m_mTime; }
        virtual time_t aTime() const { return m_aTime; }
        virtual int permissions() const { return m_perm; }

    private:
        mutable Archive::Reader::Holder m_reader;

        char *m_path;
        const char *m_title;
        time_t m_cTime;
        time_t m_mTime;
        time_t m_aTime;
        int m_perm;
        uint64_t m_size;

        Interface::Adaptor<IType> m_type;
    };


    class Dir : public Implements<IEntry, IDirectory>
    {
    public:
        Dir(const EFC::String &title, const Interface::Holder &file) :
            m_title(title),
            m_file(file),
            m_type(Module::desktop().typeOfDirectory())
        {}

        virtual ~Dir()
        {}

        Archive::Entries *entries() { return &m_entries; }

    public: /* IEntry */
        virtual const char *title() const { return m_title.c_str(); }
        virtual const char *schema() const { return "file"; }
        virtual const char *location() const { return m_file->as<IEntry>()->location(); }
        virtual const IType *type() const { return m_type; }
        virtual Interface::Holder open(IStream::Mode mode = IStream::Read) const
        {
            m_error = Error(EISDIR);
            return Interface::Holder();
        }

    public: /* IDirectory */
        virtual const_iterator begin() const { return std_iterator<Archive::Entries>(m_entries.begin()); }
        virtual const_iterator end() const { return std_iterator<Archive::Entries>(m_entries.end()); }

        virtual bool exists(const char *name) const { return false; }
        virtual Interface::Holder entry(const char *name, const IType *type = NULL, bool create = false) { return Interface::Holder(); }

        virtual bool copy(const Progress &callback, const Interface::Holder &file, bool move = false) { return false; }
        virtual bool rename(const Interface::Holder &file, const char *name) { return false; }
        virtual bool remove(const Interface::Holder &file) { return false; }

        virtual const Error &lastError() const { return m_error; }

    private:
        EFC::String m_title;
        Interface::Holder m_file;
        Archive::Entries m_entries;
        Interface::Adaptor<IType> m_type;
        mutable Error m_error;
    };


}


Archive::Archive(const Interface::Holder &file) :
    ExtendsBy(file),
    m_password(NULL),
    m_lastError(&m_error)
{
    ASSERT(file.isValid());
}

Archive::~Archive()
{
    if (m_password)
        free(m_password);
}

Archive::const_iterator Archive::end() const
{
    return std_iterator<Entries>(m_entries.end());
}

bool Archive::exists(const char *name) const
{
    return false;
}

Interface::Holder Archive::entry(const char *name, const IType *type, bool create)
{
    return Interface::Holder();
}

bool Archive::copy(const Progress &callback, const Interface::Holder &file, bool move)
{
    return false;
}

bool Archive::rename(const Interface::Holder &file, const char *name)
{
    m_lastError = &m_error;
    m_error = Error(0);
    return false;
}

bool Archive::remove(const Interface::Holder &file)
{
    m_lastError = &m_error;
    m_error = Error(0);
    return false;
}

const char *Archive::password() const
{
    return m_password;
}

void Archive::setPassword(const char *value)
{
    if (m_password)
        free(m_password);

    m_password = strdup(value);
}

const Error &Archive::lastError() const
{
    return *m_lastError;
}

bool Archive::process(ReaderHolder &reader)
{
    if (reader.isValid() && reader->open())
    {
        Interface::Holder entry;
        Interface::Holder dir;
        EFC::String key;
        Entries *entries;
        char *path;
        char *sep;

        do
            if (reader->next())
            {
                entries = &m_entries;
                entry.reset(new (std::nothrow) ArchiveEntry(reader));

                if (LIKELY(entry.isValid() == true))
                {
                    path = const_cast<char *>(entry.as<ArchiveEntry>()->location());

                    while (sep = ::strchr(path, '/'))
                    {
                        *sep = 0;
                        Entries::iterator lb = entries->lower_bound(key = path);
                        *sep = '/';
                        path = sep + 1;

                        if (lb != entries->end() && !(entries->key_comp()(key, lb->first)))
                            entries = lb->second.as<Dir>()->entries();
                        else
                        {
                            dir.reset(new (std::nothrow) Dir(key, original()));

                            if (UNLIKELY(dir.isValid() == false))
                            {
                                reader->close();
                                reader.reset();
                                m_entries.clear();
                                return false;
                            }

                            entries->insert(lb, Entries::value_type(key, dir));
                            entries = dir.as<Dir>()->entries();
                        }
                    }

                    entry.as<ArchiveEntry>()->initType();

                    if ((dir = Module::open(entry)).isValid())
                        entry = dir;

                    entries->insert(Entries::value_type(path, entry));
                }
                else
                {
                    reader->close();
                    reader.reset();
                    m_entries.clear();
                    return false;
                }
            }
            else
            {
                reader->close();
                reader.reset();
                break;
            }
        while(true);
    }

    return true;
}


Archive::Reader::Reader(const Interface::Holder &file, const char *password) :
    m_file(file),
    m_password(password ? ::strdup(password) : NULL)
{}

Archive::Reader::~Reader()
{
    if (m_password)
        ::free(m_password);
}

void Archive::Reader::setPassword(const char *value)
{
    if (m_password)
        ::free(m_password);

    m_password = ::strdup(value);
}

}}
