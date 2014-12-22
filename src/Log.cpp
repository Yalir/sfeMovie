
/*
 *  Log.cpp
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

#include "Log.hpp"
#include "Macros.hpp"
#include <iostream>
#include <SFML/System.hpp>
extern "C"
{
#include <libavutil/avutil.h>
}

namespace sfe
{
    namespace Log
    {
        static int g_logLevel = ErrorLogLevel;
        static sf::Mutex g_synchronized;
        
        void initialize()
        {
#if DEBUG
            setLogLevel(DebugLogLevel);
#else
            setLogLevel(ErrorLogLevel);
#endif
        }
        
        void setLogLevel(LogLevel level)
        {
            sf::Lock l(g_synchronized);
            g_logLevel = level;
            
            switch (level)
            {
                case DebugLogLevel:        av_log_set_level(AV_LOG_INFO); break;
                case WarningLogLevel:    av_log_set_level(AV_LOG_WARNING); break;
                case ErrorLogLevel:        av_log_set_level(AV_LOG_ERROR); break;
                case QuietLogLevel:        av_log_set_level(AV_LOG_QUIET); break;
                default: CHECK(false, "inconcistency");
            }
        }
        
        static std::string filename(const std::string& filepath)
        {
            size_t pos = filepath.find_last_of("/");
            
            if (pos != std::string::npos && pos+1 != filepath.size())
                return filepath.substr(pos+1);
            else
                return filepath;
        }
        
        void debug(const std::string& file, const std::string& message)
        {
            sf::Lock l(g_synchronized);
            
            if (g_logLevel >= DebugLogLevel)
            {
                std::cerr << "Debug: " << filename(file) << message << std::endl;
            }
        }
        
        void warning(const std::string& file, const std::string& message)
        {
            sf::Lock l(g_synchronized);
            
            if (g_logLevel >= WarningLogLevel)
            {
                std::cerr << "Warning: " << filename(file) << message << std::endl;
            }
        }
        
        void error(const std::string& file, const std::string& message)
        {
            sf::Lock l(g_synchronized);
            
            if (g_logLevel >= ErrorLogLevel)
            {
                std::cerr << "Error: " << filename(file) << message << std::endl;
            }
        }
    }
}
