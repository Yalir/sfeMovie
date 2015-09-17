
/*
 *  main.cpp
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


#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <sfeMovie/Movie.hpp>
#include <iostream>
#include <algorithm>


/*
 * Here is a little use sample for sfeMovie.
 * It'll open and display the movie specified by MOVIE_FILE above.
 *
 * This sample implements basic controls as follow:
 *  - Escape key to exit
 *  - Space key to play/pause the movie playback
 */

void my_pause()
{
#ifdef SFML_SYSTEM_WINDOWS
    system("PAUSE");
#endif
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " movie_path" << std::endl;
        my_pause();
        return 1;
    }
    
    std::string mediaFile = std::string(argv[1]);
    std::cout << "Going to open movie file \"" << mediaFile << "\"" << std::endl;
    
    sfe::Movie movie;
    if (!movie.openFromFile(mediaFile))
    {
        my_pause();
        return 1;
    }
    
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    float width = std::min(static_cast<float>(desktopMode.width), movie.getSize().x);
    float height = std::min(static_cast<float>(desktopMode.height), movie.getSize().y);
    
    // For audio files, there is no frame size, set a minimum:
    if (width * height < 1.f)
    {
        width = std::max(width, 250.f);
        height = std::max(height, 40.f);
    }

    // Create window
    sf::RenderWindow window(sf::VideoMode(width, height), "sfeMovie Player",
                            sf::Style::Close | sf::Style::Resize);
    
    // Scale movie to the window drawing area and enable VSync
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    
    movie.fit(0, 0, width, height);
    movie.play();
    
    while (window.isOpen())
    {
        sf::Event ev;
        while (window.pollEvent(ev))
        {
            // Window closure
            if (ev.type == sf::Event::Closed ||
                (ev.type == sf::Event::KeyPressed &&
                 ev.key.code == sf::Keyboard::Escape))
            {
                window.close();
            }
            
            if (ev.type == sf::Event::KeyPressed)
            {
                switch (ev.key.code)
                {
                    case sf::Keyboard::Space:
                        if (movie.getStatus() == sfe::Playing)
                            movie.pause();
                        else
                            movie.play();
                        break;
                    default:
                        break;
                }
            }
            else if (ev.type == sf::Event::Resized)
            {
                movie.fit(0, 0, window.getSize().x, window.getSize().y);
                window.setView(sf::View(sf::FloatRect(0, 0, (float)window.getSize().x, (float)window.getSize().y)));
            }
        }
        
        movie.update();
        
        // Render movie
        window.clear();
        window.draw(movie);
        window.display();
    }
    
    return 0;
}
