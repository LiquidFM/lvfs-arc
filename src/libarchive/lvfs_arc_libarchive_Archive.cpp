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

#include <brolly/assert.h>

#include <archive.h>
#include <archive_entry.h>

#include <cstring>


namespace LVFS {
namespace Arc {
namespace LibArchive {

namespace {
    class ArchiveReader : public Archive::Reader
    {
    public:
        enum { BlockSize = 65536 };

    public:
        ArchiveReader(const Interface::Holder &file, const char *password) :
            Reader(file, password),
            m_archive(NULL),
            m_entry(NULL)
        {}

        virtual ~ArchiveReader()
        {
            close();
        }

        virtual bool isOpen() const
        {
            return m_archive != NULL;
        }

        virtual bool open()
        {
            ASSERT(m_archive == NULL);
            m_archive = archive_read_new();

            if (LIKELY(m_archive != NULL))
            {
                archive_read_support_filter_all(m_archive);
                archive_read_support_format_all(m_archive);

                if (LIKELY(archive_read_open2(m_archive, this, open, read, skip, close) == ARCHIVE_OK))
                    return true;
                else
                    close();
            }

            return false;
        }

        virtual size_t read(void *buffer, size_t size)
        {
            return archive_read_data(m_archive, buffer, size);
        }

        virtual void close()
        {
            archive_read_free(m_archive);
            m_archive = NULL;
            m_entry = NULL;
            m_file.reset();
        }

        virtual bool next()
        {
            while (archive_read_next_header(m_archive, &m_entry) == ARCHIVE_OK)
                if (::archive_entry_pathname(m_entry)[strlen(::archive_entry_pathname(m_entry)) - 1] != '/')
                    return true;

            return false;
        }

        virtual const char *archive_entry_pathname() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_pathname(m_entry);
        }

        virtual time_t archive_entry_ctime() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_birthtime(m_entry);
        }

        virtual time_t archive_entry_mtime() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_mtime(m_entry);
        }

        virtual time_t archive_entry_atime() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_atime(m_entry);
        }

        virtual mode_t archive_entry_perm() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_perm(m_entry);
        }

        virtual int64_t archive_entry_size() const
        {
            ASSERT(m_entry != NULL);
            return ::archive_entry_size(m_entry);
        }

    private:
        static int open(struct archive *archive, void *_client_data)
        {
            static int archive_res[2] = { ARCHIVE_FAILED, ARCHIVE_OK };
            ArchiveReader *self = static_cast<ArchiveReader *>(_client_data);
            return archive_res[(self->m_file = self->file()->as<IEntry>()->open()).isValid()];
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

            if (self->m_file->seek(request, IStream::FromCurrent))
                return request;
            else
                return 0;
        }

        static int64_t seek(struct archive *archive, void *_client_data, int64_t offset, int whence)
        {
            ArchiveReader *self = static_cast<ArchiveReader *>(_client_data);

            if (self->m_file->seek(offset, static_cast<IStream::Whence>(whence)))
                return offset;
            else
                return ARCHIVE_FATAL;
        }

        static int close(struct archive *archive, void *_client_data)
        {
            static_cast<ArchiveReader *>(_client_data)->m_file.reset();
            return ARCHIVE_OK;
        }

    private:
        Interface::Adaptor<IStream> m_file;
        mutable struct archive *m_archive;
        mutable struct archive_entry *m_entry;
        char m_buffer[BlockSize];
    };
}


Archive::Archive(const Interface::Holder &file) :
    Arc::Archive(file)
{}

Archive::~Archive()
{}

Archive::const_iterator Archive::begin() const
{
    ArchiveReader::Holder reader(new (std::nothrow) ArchiveReader(original(), password()));
    const_cast<Archive *>(this)->process(reader);
    return std_iterator<Entries>(entries().begin());
}

}}}
