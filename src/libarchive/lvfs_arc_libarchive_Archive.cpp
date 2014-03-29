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


namespace LVFS {
namespace Arc {
namespace LibArchive {

File::File(const Interface::Holder &file) :
    m_file(file),
    m_lastError(m_error)
{
    ASSERT(m_file.isValid());
}

File::~File()
{}

int File::permissions() const
{
    return m_file->as<IPermissions>()->permissions();
}

bool File::open()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->open();
}

size_t File::read(void *buffer, size_t size)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->read(buffer, size);
}

size_t File::write(const void *buffer, size_t size)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->write(buffer, size);
}

bool File::seek(long offset, Whence whence)
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->seek(offset, whence);
}

bool File::flush()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->flush();
}

void File::close()
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    file->close();
}

uint64_t File::size() const
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->size();
}

size_t File::position() const
{
    Interface::Adaptor<IFile> file(m_file);
    ASSERT(file.isValid());
    m_lastError = &file->lastError();

    return file->position();
}

File::const_iterator File::begin() const
{

}

File::const_iterator File::end() const
{

}

Interface::Holder File::entry(const char *name) const
{

}

bool File::rename(const Interface::Holder &file, const char *name)
{
    m_error = Error(0);
    return false;
}

bool File::remove(const Interface::Holder &file)
{
    m_error = Error(0);
    return false;
}

const char *File::title() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->title();
}

const char *File::type() const
{
    Interface::Adaptor<IEntry> file(m_file);
    ASSERT(file.isValid());
    return file->type();
}

const Error &File::lastError() const
{
    return *m_lastError;
}

}}}
