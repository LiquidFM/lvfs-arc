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

#include "lvfs_arc_libarchive_Archive.h"

#include <lvfs/Module>
#include <lvfs/IProperties>
#include <brolly/assert.h>

#include <cstdlib>
#include <cstring>
#include <archive.h>
#include <archive_entry.h>

#include <errno.h>


namespace LVFS {
namespace Arc {
namespace LibArchive {
namespace {


    class ArchiveReader;
    typedef ::EFC::Holder<ArchiveReader> ArchiveReaderHolder;

    class ArchiveReader : public ArchiveReaderHolder::Data
    {
    public:
        enum { BlockSize = 65536 };
        typedef ArchiveReaderHolder Holder;

    public:
        ArchiveReader(const Interface::Holder &file, const char *password) :
            m_count(0),
            m_holder(file),
            m_file(),
            m_password(password ? strdup(password) : NULL),
            m_archive(NULL),
            m_entry(NULL)
        {}

        ~ArchiveReader()
        {
            archive_read_free(m_archive);

            if (m_password)
                free(m_password);
        }

        inline const char *password() const { return m_password; }
        inline void setPassword(const char *value)
        {
            if (m_password)
                free(m_password);

            m_password = strdup(value);
        }

        inline bool open()
        {
            ASSERT(m_archive == NULL);
            m_archive = archive_read_new();

            if (LIKELY(m_archive != NULL))
            {
                archive_read_support_filter_all(m_archive);
                archive_read_support_format_all(m_archive);

                if (LIKELY(archive_read_open2(m_archive, this, open, read, skip, close) == ARCHIVE_OK))
                {
                    m_count = 1;
                    return true;
                }
                else
                    close();
            }

            return false;
        }

        inline size_t read(void *buffer, size_t size)
        {
            return archive_read_data(m_archive, buffer, size);
        }

        inline size_t write(const void *buffer, size_t size)
        {
            return 0;
        }

        inline void close()
        {
            archive_read_free(m_archive);
            m_archive = NULL;
            m_entry = NULL;
        }

        inline bool find(const char *path) const
        {
            if (m_entry != NULL)
            {
                ASSERT(strcmp(path, archive_entry_pathname(m_entry)) == 0);
                return true;
            }
            else
                while (archive_read_next_header(m_archive, &m_entry) == ARCHIVE_OK)
                    if (strcmp(path, archive_entry_pathname(m_entry)) == 0)
                        return true;

            return false;
        }

        inline struct archive_entry *next()
        {
            while (archive_read_next_header(m_archive, &m_entry) == ARCHIVE_OK)
                if (archive_entry_pathname(m_entry)[strlen(archive_entry_pathname(m_entry)) - 1] != '/')
                    return m_entry;

            return NULL;
        }

    private:
        static int open(struct archive *archive, void *_client_data)
        {
            static int archive_res[2] = { ARCHIVE_FAILED, ARCHIVE_OK };
            ArchiveReader *self = static_cast<ArchiveReader *>(_client_data);
            return archive_res[(self->m_file = self->m_holder->as<IEntry>()->open()).isValid()];
        }

        static ssize_t read(struct archive *archive, void *_client_data, const void **_buffer)
        {
            ArchiveReader *self = static_cast<ArchiveReader *>(_client_data);
            (*_buffer) = self->m_buffer;
            return self->m_file->read(self->m_buffer, BlockSize);
        }

        static int64_t skip(struct archive *archive, void *_client_data, int64_t request)
        {
            ArchiveReader *self = static_cast<ArchiveReader *>(_client_data);

            if (self->m_file->seek(request, IFile::FromCurrent))
                return request;
            else
                return 0;
        }

        static int close(struct archive *archive, void *_client_data)
        {
            static_cast<ArchiveReader *>(_client_data)->m_file.reset();
            return ARCHIVE_OK;
        }

    private:
        int m_count;
        Interface::Holder m_holder;
        Interface::Adaptor<IFile> m_file;
        char *m_password;
        mutable struct archive *m_archive;
        mutable struct archive_entry *m_entry;
        char m_buffer[BlockSize];
    };


    class ArchiveEntry : public Implements<IEntry, IFile, IProperties>
    {
    public:
        ArchiveEntry(const ArchiveReader::Holder &reader, struct archive_entry *entry) :
            m_reader(reader),
            m_path(strdup(archive_entry_pathname(entry))),
            m_cTime(archive_entry_birthtime(entry)),
            m_mTime(archive_entry_mtime(entry)),
            m_aTime(archive_entry_atime(entry)),
            m_perm(archive_entry_perm(entry)),
            m_size(archive_entry_size(entry)),
            m_type(NULL)
        {
            ASSERT(reader.isValid());
        }

        virtual ~ArchiveEntry()
        {
            free(m_path);
        }

        void initType()
        {
            m_typeHolder = Module::desktop().typeOfFile(this);
            ASSERT(m_typeHolder.isValid());
            m_type = m_typeHolder->as<IType>();
        }

        virtual const char *title() const { return m_path; }
        virtual const char *schema() const { return "file"; }
        virtual const char *location() const { return m_path; }
        virtual const IType *type() const { return m_type; }
        virtual Interface::Holder open(IFile::Mode mode) const
        {
            if (mode == IFile::Read && m_reader->find(m_path))
                return Interface::Holder::fromRawData(const_cast<ArchiveEntry *>(this));
            else
                return Interface::Holder();
        }

        virtual size_t read(void *buffer, size_t size) { return m_reader->read(buffer, size); }
        virtual size_t write(const void *buffer, size_t size) { m_error = Error(ENOENT); return false; }
        virtual bool advise(off_t offset, off_t len, Advise advise) { return false; }
        virtual bool seek(long offset, Whence whence) { m_error = Error(ENOENT); return false; }
        virtual bool flush() { m_error = Error(ENOENT); return false; }

        virtual off64_t size() const { return m_size; }
        virtual time_t cTime() const { return m_cTime; }
        virtual time_t mTime() const { return m_mTime; }
        virtual time_t aTime() const { return m_aTime; }
        virtual int permissions() const { return m_perm; }

        virtual const Error &lastError() const { return m_error; }

    private:
        mutable Error m_error;
        ArchiveReader::Holder m_reader;

        char *m_path;
        time_t m_cTime;
        time_t m_mTime;
        time_t m_aTime;
        int m_perm;
        uint64_t m_size;

        const IType *m_type;
        Interface::Holder m_typeHolder;
    };


    class Iterator : public Archive::const_iterator
    {
    public:
        Iterator() :
            Archive::const_iterator(new (std::nothrow) Imp())
        {}

        Iterator(const Interface::Holder &file, const char *password) :
            Archive::const_iterator(new (std::nothrow) Imp(file, password))
        {}

    protected:
        class Imp : public Archive::const_iterator::Implementation
        {
            PLATFORM_MAKE_NONCOPYABLE(Imp)
            PLATFORM_MAKE_NONMOVEABLE(Imp)

        public:
            Imp()
            {}

            Imp(const Interface::Holder &file, const char *password) :
                m_reader(new (std::nothrow) ArchiveReader(file, password))
            {
                if (m_reader.isValid() && m_reader->open())
                    next();
                else
                    m_reader.reset();
            }

            virtual ~Imp()
            {}

            virtual bool isEqual(const Holder &other) const
            {
                return m_reader == other.as<Imp>()->m_reader;
            }

            virtual reference asReference() const
            {
                return m_res;
            }

            virtual pointer asPointer() const
            {
                return &m_res;
            }

            virtual void next()
            {
                m_res.reset();

                if (struct archive_entry *e = m_reader->next())
                {
                    m_res.reset(new (std::nothrow) ArchiveEntry(m_reader, e));

                    if (LIKELY(m_res.isValid() == true))
                    {
                        m_res.as<ArchiveEntry>()->initType();

                        Interface::Holder modded = Module::open(m_res);
                        if (modded.isValid())
                            m_res = modded;

                        return;
                    }
                }

                m_reader.reset();
            }

        private:
            ArchiveReader::Holder m_reader;
            Interface::Holder m_res;
        };
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

Archive::const_iterator Archive::begin() const
{
    return Iterator(original(), m_password);
}

Archive::const_iterator Archive::end() const
{
    return Iterator();
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

}}}
