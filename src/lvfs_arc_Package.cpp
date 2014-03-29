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

#include "lvfs_arc_Package.h"
#include "lvfs_arc_LibArchive.h"


namespace LVFS {
namespace Arc {

Package::Package()
{}

Package::~Package()
{}

const char *Package::name() const
{
    return "Arc";
}

const Package::DataPlugin *Package::dataPlugins() const
{
    static const LibArchive libArchive;

    static const DataPlugin types[] =
    {
        { "application/zip",    &libArchive },
        { "application/x-gzip", &libArchive },
        NULL
    };

    static const DataPlugin *res[] =
    {
        &types[0],
        &types[1],
        NULL
    };

    return res[0];
}

const Package::RootPlugin *Package::rootPlugins() const
{
    return NULL;
}

}}
