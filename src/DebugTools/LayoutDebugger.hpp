
/*
 *  LayoutDebugger.hpp
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


#ifndef LAYOUT_DEBUGGER_HPP
#define LAYOUT_DEBUGGER_HPP

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

/** Class for debugging the layout of drawable objects
 *
 * It can be used for any type that has a getGlobalBounds() method.
 * It'll draw the object borders and diagonals to show it's center point
 *
 * To use it, create the debugger, bind it to a drawable and draw it
 */
template <typename T>
class LayoutDebugger : public sf::Drawable, public sf::Transformable
{
public:
    /** Default constructor
     *
     * Create a debugger that'll outline the bound object with the given color
     * No ownership is taken on the drawable
     */
    LayoutDebugger(const sf::Color& color = sf::Color::Red,
                   const T* drawable = nullptr);
    
    /** Bind this debugger to an object to show its borders and center
     *
     * No ownership is taken on the drawable
     */
    void bind(const T* drawable);
private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
    
    sf::VertexArray m_lines;
    const T* m_debuggedDrawable;
    sf::Color m_debugColor;
};


template <typename T>
LayoutDebugger<T>::LayoutDebugger(const sf::Color& color,
                                  const T* drawable)
: m_lines()
, m_debuggedDrawable(drawable)
, m_debugColor(color)
{
}

template <typename T>
void LayoutDebugger<T>::bind(const T* drawable)
{
    m_debuggedDrawable = drawable;
}

template <typename T>
void LayoutDebugger<T>::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (m_debuggedDrawable)
    {
        sf::FloatRect bounds = m_debuggedDrawable->getGlobalBounds();
        
        sf::Vertex points[] =
        {
            sf::Vertex(sf::Vector2f(bounds.left, bounds.top), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left + bounds.width, bounds.top), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left, bounds.top + bounds.height), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left, bounds.top), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left, bounds.top + bounds.height), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height), m_debugColor),
            sf::Vertex(sf::Vector2f(bounds.left + bounds.width, bounds.top), m_debugColor)
        };
        
        target.draw(points, 8, sf::LinesStrip);
    }
}

#endif
