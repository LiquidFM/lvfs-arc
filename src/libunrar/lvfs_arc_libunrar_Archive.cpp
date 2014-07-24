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

#include <lvfs/Module>
#include <brolly/assert.h>

#include <wchar.h>
#include <libunrar/rar.hpp>
#include <libunrar/dll.hpp>
#include <libunrar/timefn.hpp>

#include <cstdio>


namespace LVFS {
namespace Arc {
namespace LibUnrar {
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
            m_password(password ? strdup(password) : NULL),
            m_fileHolder(file),
            m_file(m_fileHolder),
            m_tmpFile(NULL)
        {
            memset(&m_archiveData, 0, sizeof(m_archiveData));
            memset(&m_archiveInfo, 0, sizeof(m_archiveInfo));

            Interface::Adaptor<IEntry> entry(m_fileHolder);
            m_archiveData.ArcName = const_cast<char *>(entry->location());
            m_archiveData.OpenMode = RAR_OM_EXTRACT;
            m_archiveData.Callback = unrarcallback;
            m_archiveData.UserData = reinterpret_cast<LPARAM>(this);
        }

        virtual ~ArchiveReader()
        {
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

            if (m_archive = RAROpenArchiveEx(&m_archiveData))
            {
                RARSetCallback(m_archive, unrarcallback, reinterpret_cast<LPARAM>(this));

                if (m_password)
                    RARSetPassword(m_archive, m_password);

                m_count = 1;
                return true;
            }

            return false;
        }

        virtual size_t read(void *buffer, size_t size)
        {
            int res = 0;

            if (m_tmpFile)
                return fread(buffer, 1, size, m_tmpFile);
            else
                if ((m_tmpFile = tmpfile()) && (res = RARProcessFile(m_archive, RAR_EXTRACT, NULL, NULL, true)) == 0)
                {
                    fseek(m_tmpFile, 0, SEEK_SET);
                    return fread(buffer, 1, size, m_tmpFile);
                }

            return 0;
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
                if (m_tmpFile)
                    fclose(m_tmpFile);

                RARCloseArchive(m_archive);

                m_archive = NULL;
                m_tmpFile = NULL;

                memset(&m_archiveInfo, 0, sizeof(m_archiveInfo));
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
            if (m_archiveInfo.FileName[0] != 0)
            {
                ASSERT(strcmp(path, m_archiveInfo.FileName) == 0);
                return true;
            }
            else
                while (RARReadHeaderEx(m_archive, &m_archiveInfo) == 0)
                    if (strcmp(path, m_archiveInfo.FileName) == 0)
                        return true;
                    else
                        RARProcessFile(m_archive, RAR_SKIP, NULL, NULL);

            return false;
        }

        inline struct RARHeaderDataEx *next()
        {
            if (m_archiveInfo.FileName[0] != 0)
                if (m_tmpFile)
                {
                    fclose(m_tmpFile);
                    m_tmpFile = NULL;
                }
                else
                    RARProcessFile(m_archive, RAR_SKIP, NULL, NULL);

            if (RARReadHeaderEx(m_archive, &m_archiveInfo) == 0)
                return &m_archiveInfo;

            return NULL;
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
        int m_count;
        void *m_archive;
        char *m_password;
        Interface::Holder m_fileHolder;
        Interface::Adaptor<IFile> m_file;
        struct RAROpenArchiveDataEx m_archiveData;
        struct RARHeaderDataEx m_archiveInfo;

        FILE *m_tmpFile;
    };


    class ArchiveEntry : public Implements<IFile, IEntry, IFsFile>
    {
    public:
        ArchiveEntry(const ArchiveReader::Holder &reader, struct RARHeaderDataEx *entry) :
            m_reader(reader),
            m_entry(*entry),
            m_type(NULL)
        {
            ASSERT(reader.isValid());
            m_typeHolder = Module::desktop().typeOfFile(this, m_entry.FileName);
            ASSERT(m_typeHolder.isValid());
            m_type = m_typeHolder->as<IType>();
        }

        virtual ~ArchiveEntry()
        {
    //        free(m_path);
        }

        virtual bool open()
        {
            if (m_reader->open())
            {
                if (m_reader->find(m_entry.FileName))
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

        virtual uint64_t size() const { return PLATFORM_MAKE_QWORD(m_entry.UnpSizeHigh, m_entry.UnpSize); }
        virtual size_t position() const { m_error = Error(ENOENT); return 0; }
        virtual const Error &lastError() const { return m_error; }

        virtual const char *title() const { return m_entry.FileName; }
        virtual const char *schema() const { return "file"; }
        virtual const char *location() const { return m_entry.FileName; }
        virtual const IType *type() const { return m_type; }

        virtual time_t cTime() const { return 0; }
        virtual time_t mTime() const { return 0; }
        virtual time_t aTime() const { return 0; }

        virtual int permissions() const { return 0; }
        virtual bool setPermissions(int value) { m_error = Error(ENOENT); return false; }

    private:
        mutable Error m_error;
        ArchiveReader::Holder m_reader;
        struct RARHeaderDataEx m_entry;

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

                if (struct RARHeaderDataEx *e = m_reader->next())
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
