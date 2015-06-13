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

#include "lvfs_arc_libunrar_Archive.h"

#include <brolly/assert.h>

#include <wchar.h>
#include <libunrar/rar.hpp>
#include <libunrar/dll.hpp>
#include <libunrar/timefn.hpp>


namespace LVFS {
namespace Arc {
namespace LibUnrar {

namespace {
    class ArchiveReader : public Archive::Reader
    {
    public:
        ArchiveReader(const Interface::Holder &file, const char *password) :
            Reader(file, password),
            m_archive(NULL),
            m_tmpFile(NULL)
        {
            ::memset(&m_archiveData, 0, sizeof(m_archiveData));
            ::memset(&m_archiveInfo, 0, sizeof(m_archiveInfo));

            m_archiveData.ArcName = const_cast<char *>(file->as<IEntry>()->location());
            m_archiveData.OpenMode = RAR_OM_EXTRACT;
            m_archiveData.Callback = unrarcallback;
            m_archiveData.UserData = reinterpret_cast<LPARAM>(this);
        }

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

            if (m_archive = RAROpenArchiveEx(&m_archiveData))
            {
                RARSetCallback(m_archive, unrarcallback, reinterpret_cast<LPARAM>(this));

                if (password())
                    RARSetPassword(m_archive, const_cast<char *>(password()));

                return true;
            }

            return false;
        }

        virtual size_t read(void *buffer, size_t size)
        {
            int res = 0;

            if (m_tmpFile)
                return ::fread(buffer, 1, size, m_tmpFile);
            else
                if ((m_tmpFile = ::tmpfile()) && (res = RARProcessFile(m_archive, RAR_EXTRACT, NULL, NULL, true)) == 0)
                {
                    ::fseek(m_tmpFile, 0, SEEK_SET);
                    return ::fread(buffer, 1, size, m_tmpFile);
                }

            return 0;
        }

        virtual void close()
        {
            if (m_tmpFile)
                ::fclose(m_tmpFile);

            RARCloseArchive(m_archive);

            m_archive = NULL;
            m_tmpFile = NULL;

            ::memset(&m_archiveInfo, 0, sizeof(m_archiveInfo));
        }

        virtual bool next()
        {
            if (m_archiveInfo.FileName[0] != 0)
                if (m_tmpFile)
                {
                    ::fclose(m_tmpFile);
                    m_tmpFile = NULL;
                }
                else
                    RARProcessFile(m_archive, RAR_SKIP, NULL, NULL);

            if (RARReadHeaderEx(m_archive, &m_archiveInfo) == 0)
                return true;

            return false;
        }

        virtual const char *archive_entry_pathname() const
        {
            ASSERT(m_archive != NULL);
            return m_archiveInfo.FileName;
        }

        virtual time_t archive_entry_ctime() const
        {
            ASSERT(m_archive != NULL);
            return 0;
        }

        virtual time_t archive_entry_mtime() const
        {
            ASSERT(m_archive != NULL);
            return 0;
        }

        virtual time_t archive_entry_atime() const
        {
            ASSERT(m_archive != NULL);
            return 0;
        }

        virtual mode_t archive_entry_perm() const
        {
            ASSERT(m_archive != NULL);
            return 0;
        }

        virtual int64_t archive_entry_size() const
        {
            ASSERT(m_archive != NULL);
            return PLATFORM_MAKE_QWORD(m_archiveInfo.UnpSizeHigh, m_archiveInfo.UnpSize);
        }

    private:
        static int CALLBACK unrarcallback(UINT msg, LPARAM userData, LPARAM p1, LPARAM p2)
        {
            ArchiveReader *self = reinterpret_cast<ArchiveReader *>(userData);

            switch (msg)
            {
                case UCM_PROCESSDATA:
                {
                    if (fwrite(reinterpret_cast<void *>(p1), 1, p2, self->m_tmpFile) != (size_t)p2)
                        return -1;

                    break;
                }

                default:
                    break;
            }

            return ERAR_SUCCESS;
        }

    private:
        Interface::Adaptor<IStream> m_file;

        mutable void *m_archive;
        mutable struct RAROpenArchiveDataEx m_archiveData;
        mutable struct RARHeaderDataEx m_archiveInfo;

        FILE *m_tmpFile;
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
