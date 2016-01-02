
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
#include "UserInterface.hpp"
#include "StreamSelector.hpp"
#include "MediaInfo.hpp"


/*
 * Here is a little use sample for sfeMovie.
 * It'll open and display the movie specified by MOVIE_FILE above.
 *
 * This sample implements basic controls as follow:
 *  - Escape key to exit
 *  - Space key to play/pause the movie playback
 *  - S key to stop and go back to the beginning of the movie
 *  - F key to toggle between windowed and fullscreen mode
 *  - A key to select the next audio stream
 *  - V key to select the next video stream
 */

void my_pause()
{
#ifdef SFML_SYSTEM_WINDOWS
    system("PAUSE");
#endif
}

void displayShortcuts()
{
    std::cout << "Shortcuts:\n"
    << "\tSpace - Play / pause\n"
    << "\tS - Stop\n"
    << "\tH - Hide / show user controls and mouse cursor\n"
    << "\tF - Toggle fullscreen\n"
    << "\tI - Log media info and current state\n"
    << "\tAlt + V - Select next video stream\n"
    << "\tAlt + A - Select next audio stream\n"
    << "\tAlt + S - Select next subtitle stream"
    << std::endl;
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
    
    bool fullscreen = false;
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
    
    UserInterface ui(window, movie);
    StreamSelector selector(movie);
    displayShortcuts();

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
                        
                    case sf::Keyboard::A:
                        if (ev.key.alt)
                            selector.selectNextStream(sfe::Audio);
                        break;
                        
                    case sf::Keyboard::F:
                        fullscreen = !fullscreen;
                        
                        if (fullscreen)
                            window.create(desktopMode, "sfeMovie Player", sf::Style::Fullscreen);
                        else
                            window.create(sf::VideoMode(width, height), "sfeMovie Player",
                                          sf::Style::Close | sf::Style::Resize);
                        
                        window.setFramerateLimit(60);
                        window.setVerticalSyncEnabled(true);
                        movie.fit(0, 0, (float)window.getSize().x, (float)window.getSize().y);
                        ui.applyProperties();
                        break;
                        
                    case sf::Keyboard::H:
                        ui.toggleVisible();
                        break;
                        
                    case sf::Keyboard::I:
                        displayMediaInfo(movie);;
                        break;
                        
                    case sf::Keyboard::S:
                        if (ev.key.alt)
                            selector.selectNextStream(sfe::Subtitle);
                        else
                            movie.stop();
                        break;
                        
                    case sf::Keyboard::V:
                        if (ev.key.alt)
                            selector.selectNextStream(sfe::Video);
                    default:
                        break;
                }
            }
            else if (ev.type == sf::Event::MouseWheelMoved)
            {
                float volume = movie.getVolume() + 10 * ev.mouseWheel.delta;
                volume = std::min(volume, 100.f);
                volume = std::max(volume, 0.f);
                movie.setVolume(volume);
                std::cout << "Volume changed to " << int(volume) << "%" << std::endl;
            }
            else if (ev.type == sf::Event::Resized)
            {
                movie.fit(0, 0, window.getSize().x, window.getSize().y);
                window.setView(sf::View(sf::FloatRect(0, 0, (float)window.getSize().x, (float)window.getSize().y)));
            }
            else if (ev.type == sf::Event::MouseButtonPressed ||
                     (ev.type == sf::Event::MouseMoved && sf::Mouse::isButtonPressed(sf::Mouse::Left)))
            {
                int xPos = 0;
                
                if (ev.type == sf::Event::MouseButtonPressed)
                    xPos = ev.mouseButton.x;
                else if (ev.type == sf::Event::MouseMoved)
                    xPos = ev.mouseMove.x;
                
                float ratio = static_cast<float>(xPos) / window.getSize().x;
                sf::Time targetTime = ratio * movie.getDuration();
                movie.setPlayingOffset(targetTime);
            }
        }
        
        movie.update();
        
        // Render movie
        window.clear();
        window.draw(movie);
        ui.draw();
        window.display();
    }
    
    return 0;
}
