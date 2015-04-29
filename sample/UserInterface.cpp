
/*
 *  UI.cpp
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

#include "UserInterface.hpp"

namespace
{
    const int kHorizontalMargin = 10;
    const int kVerticalMargin = 10;
    const int kTimelineBackgroundHeight = 20;
    const int kTimelineInnerMargin = 4;
}

UserInterface::UserInterface(sf::RenderWindow& window, const sfe::Movie& movie)
: m_window(window)
, m_movie(movie)
, m_visible(true)
{
}

void UserInterface::toggleVisible()
{
    m_visible = !m_visible;
    
    applyProperties();
}

void UserInterface::applyProperties()
{
    m_window.setMouseCursorVisible(m_visible);
}

void UserInterface::draw() const
{
    if (!m_visible)
        return;
    
    sf::RectangleShape background(sf::Vector2f(m_window.getSize().x - 2 * kHorizontalMargin, kTimelineBackgroundHeight));
    background.setPosition(kHorizontalMargin, m_window.getSize().y - kTimelineBackgroundHeight - kVerticalMargin);
    background.setFillColor(sf::Color(0, 0, 0, 255/2));
    
    sf::RectangleShape border(sf::Vector2f(background.getSize().x - 2 * kTimelineInnerMargin, background.getSize().y - 2 * kTimelineInnerMargin));
    border.setPosition(background.getPosition().x + kTimelineInnerMargin, background.getPosition().y + kTimelineInnerMargin);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::White);
    border.setOutlineThickness(1.0);
    
    float fprogress = m_movie.getPlayingOffset().asSeconds() / m_movie.getDuration().asSeconds();
    sf::RectangleShape progress(sf::Vector2f(1, border.getSize().y - 2 * border.getOutlineThickness()));
    progress.setPosition(border.getPosition().x + border.getOutlineThickness() +
                         fprogress * (border.getSize().x - 2 * border.getOutlineThickness()),
                         border.getPosition().y + border.getOutlineThickness());
    progress.setFillColor(sf::Color::White);
    
    m_window.draw(background);
    m_window.draw(border);
    m_window.draw(progress);
}