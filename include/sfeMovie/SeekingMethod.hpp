
/*
 *  SeekingMethod.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef SFEMOVIE_SEEKING_METHOD_HPP
#define SFEMOVIE_SEEKING_METHOD_HPP

namespace sfe
{
    namespace SeekingMethod
    {
        enum
        {
            Accurate = 1 << 0,  //!< Seek exactly at the requested position
            Fast     = 1 << 1   //!< Disable slow algorithms
                                //!< Note that both accurate and fast means seeking to non-key frames
                                //!< which will result in incomplete images until the next key frame is reached
        };
    }
}

#endif
