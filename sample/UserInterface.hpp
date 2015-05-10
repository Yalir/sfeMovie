
/*
 *  UI.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2015 Lucas Soltic
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


#ifndef SFEMOVIE_SAMPLE_UI_HPP
#define SFEMOVIE_SAMPLE_UI_HPP

#include <SFML/Graphics/RenderWindow.hpp>
#include <sfeMovie/Movie.hpp>

class UserInterface
{
public:
    UserInterface(sf::RenderWindow& window, const sfe::Movie& movie);
    
    /** @param flag true to enable drawing of the user interface
     */
    void toggleVisible();
    
    /** Explicitely re-apply visiblity properties to the window and movie
     */
    void applyProperties();
    
    /** Draw the user interface on the window and from the movie
     * given at construction time
     */
    void draw() const;
    
private:
    sf::RenderWindow& m_window;
    const sfe::Movie& m_movie;
    bool m_visible;
};

#endif
