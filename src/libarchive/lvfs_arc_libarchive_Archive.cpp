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

    class ArchiveReader : public ArchiveReaderHolder::Data, public IFile
    {
    public:
        enum { BlockSize = 65536 };
        typedef ArchiveReaderHolder Holder;

    public:
        ArchiveReader(const Interface::Holder &file, const char *password) :
            m_count(0),
            m_archive(NULL),
            m_entry(NULL),
            m_file(file),
            m_password(password ? strdup(password) : NULL)
        {}

        virtual ~ArchiveReader()
        {
            archive_read_free(m_archive);

            if (m_password)
                free(m_password);
        }

        inline bool isValid() const { return m_file.isValid(); }

        const char *password() const { return m_password; }
        void setPassword(const char *value)
        {
            if (m_password)
                free(m_password);

            m_password = strdup(value);
        }

        virtual bool open()
        {
            if (m_archive != NULL)
            {
                ++m_count;
                return true;
            }

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

        virtual size_t read(void *buffer, size_t size)
        {
            return archive_read_data(m_archive, buffer, size);
        }

        virtual size_t write(const void *buffer, size_t size)
        {
            return 0;
        }

        virtual bool seek(long offset, Whence whence)
        {
            return false;
        }

        virtual bool flush()
        {
            return false;
        }

        virtual void close()
        {
            if (--m_count == 0)
            {
                archive_read_free(m_archive);
                m_archive = NULL;
                m_entry = NULL;
            }
        }

        virtual uint64_t size() const
        {
            return m_file->size();
        }

        virtual size_t position() const
        {
            return m_file->position();
        }

        virtual const Error &lastError() const
        {
            return m_file->lastError();
        }

        inline bool find(const char *path)
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
            return archive_res[static_cast<ArchiveReader *>(_client_data)->m_file->open()];
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
            int64_t res = self->m_file->position();

            if (self->m_file->seek(request, IFile::FromCurrent))
                return self->m_file->position() - res;
            else
                return 0;
        }

        static int close(struct archive *archive, void *_client_data)
        {
            static_cast<ArchiveReader *>(_client_data)->m_file->close();
            return ARCHIVE_OK;
        }

    private:
        int m_count;
        struct archive *m_archive;
        struct archive_entry *m_entry;
        Interface::Adaptor<IFile> m_file;
        char *m_password;
        char m_buffer[BlockSize];
    };


    class ArchiveEntry : public Implements<IFile, IEntry, IFsFile>
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
            m_typeHolder = Module::desktop().typeOfFile(this, m_path);
            ASSERT(m_typeHolder.isValid());
            m_type = m_typeHolder->as<IType>();
        }

        virtual ~ArchiveEntry()
        {
            free(m_path);
        }

        virtual bool open()
        {
            if (m_reader->open())
            {
                if (m_reader->find(m_path))
                    return true;

                m_reader->close();
            }

            return false;
        }

        virtual size_t read(void *buffer, size_t size)
        {
            return m_reader->read(buffer, size);
        }

        virtual size_t write(const void *buffer, size_t size) { m_error = Error(ENOENT); return false; }
        virtual bool seek(long offset, Whence whence) { m_error = Error(ENOENT); return false; }
        virtual bool flush() { m_error = Error(ENOENT); return false; }

        virtual void close()
        {
            m_reader->close();
        }

        virtual uint64_t size() const { return m_size; }
        virtual size_t position() const { m_error = Error(ENOENT); return 0; }
        virtual const Error &lastError() const { return m_error; }

        virtual const char *title() const { return m_path; }
        virtual const char *schema() const { return "file"; }
        virtual const char *location() const { return m_path; }
        virtual const IType *type() const { return m_type; }

        virtual time_t cTime() const { return m_cTime; }
        virtual time_t mTime() const { return m_mTime; }
        virtual time_t aTime() const { return m_aTime; }

        virtual int permissions() const { return m_perm; }
        virtual bool setPermissions(int value) { m_error = Error(ENOENT); return false; }

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
                if (m_reader.isValid() && m_reader->isValid() && m_reader->open())
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
                    return;
                }

                m_reader->close();
                m_reader.reset();
            }


        private:
            ArchiveReader::Holder m_reader;
            Interface::Holder m_res;
        };
    };

}


Archive::Archive(const Interface::Holder &file) :
    m_file(file),
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

bool Archive::open()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->open();
}

size_t Archive::read(void *buffer, size_t size)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->read(buffer, size);
}

size_t Archive::write(const void *buffer, size_t size)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->write(buffer, size);
}

bool Archive::seek(long offset, Whence whence)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->seek(offset, whence);
}

bool Archive::flush()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->flush();
}

void Archive::close()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    file->close();
}

uint64_t Archive::size() const
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->size();
}

size_t Archive::position() const
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->position();
}

Archive::const_iterator Archive::begin() const
{
    return Iterator(m_file, m_password);
}

Archive::const_iterator Archive::end() const
{
    return Iterator();
}

Interface::Holder Archive::entry(const char *name) const
{
    return Interface::Holder();
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

const char *Archive::title() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->title();
}

const char *Archive::schema() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->schema();
}

const char *Archive::location() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->location();
}

const IType *Archive::type() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->type();
}

time_t Archive::cTime() const
{
    Interface::Adaptor<IFsFile> file(m_file);
    ASSERT(file.isValid());
    return file->cTime();
}

time_t Archive::mTime() const
{
    Interface::Adaptor<IFsFile> file(m_file);
    ASSERT(file.isValid());
    return file->mTime();
}

time_t Archive::aTime() const
{
    Interface::Adaptor<IFsFile> file(m_file);
    ASSERT(file.isValid());
    return file->aTime();
}

int Archive::permissions() const
{
    Interface::Adaptor<IFsFile> file(m_file);
    ASSERT(file.isValid());
    return file->permissions();
}

bool Archive::setPermissions(int value)
{
    Interface::Adaptor<IFsFile> file(m_file);
    ASSERT(file.isValid());
    return file->setPermissions(value);
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
