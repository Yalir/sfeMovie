
#include <gtest/gtest.h>
#include "Timer.hpp"

namespace
{
    int globalCallOrder = 0;
    
    class MyObserver : public sfe::Timer::Observer {
    public:
        MyObserver(int expectedCallOrder = 0) :
        sfe::Timer::Observer(),
        m_willPlay(false),
        m_didPlay(false),
        m_didPause(false),
        m_didStop(false),
        m_expectedCallOrder(expectedCallOrder)
        {
        }
        
        void willPlay(const sfe::Timer& timer)
        {
            EXPECT_FALSE(m_didPlay);
            m_willPlay = true;
            
            if (m_expectedCallOrder > 0)
            {
                EXPECT_EQ(++globalCallOrder, m_expectedCallOrder);
            }
        }
        
        void didPlay(const sfe::Timer& timer, sfe::Status previousStatus)
        {
            EXPECT_TRUE(m_willPlay);
            m_didPlay = true;
        }
        
        void didPause(const sfe::Timer& timer, sfe::Status previousStatus)
        {
            m_didPause = true;
        }
        
        void didStop(const sfe::Timer& timer, sfe::Status previousStatus)
        {
            m_didStop = true;
        }
        
        bool m_willPlay, m_didPlay, m_didPause, m_didStop;
        int m_expectedCallOrder;
    };
}
    
TEST(TimerTest, Base)
{
    sfe::Timer timer;
    
    EXPECT_EQ(timer.getOffset(), sf::Time::Zero);
    EXPECT_EQ(timer.getStatus(), sfe::Stopped);
    
    EXPECT_NO_THROW(timer.play());
    EXPECT_EQ(timer.getStatus(), sfe::Playing);
    EXPECT_THROW(timer.play(), std::runtime_error);
    
    EXPECT_NO_THROW(timer.pause());
    EXPECT_EQ(timer.getStatus(), sfe::Paused);
    EXPECT_THROW(timer.pause(), std::runtime_error);
    
    EXPECT_NO_THROW(timer.stop());
    EXPECT_EQ(timer.getStatus(), sfe::Stopped);
    EXPECT_THROW(timer.stop(), std::runtime_error);
    
    EXPECT_EQ(timer.getOffset(), sf::Time::Zero);
    
    timer.play();
    sf::sleep(sf::seconds(1));
    EXPECT_NE(timer.getOffset(), sf::Time::Zero);
}

TEST(TimerTest, Notifications)
{
    sfe::Timer timer;
    MyObserver obs1, obs2;
    
    timer.addObserver(obs1);
    timer.addObserver(obs2);
    
    EXPECT_FALSE(obs1.m_willPlay);
    EXPECT_FALSE(obs1.m_didPlay);
    EXPECT_FALSE(obs1.m_didPause);
    EXPECT_FALSE(obs1.m_didStop);
    EXPECT_FALSE(obs2.m_willPlay);
    EXPECT_FALSE(obs2.m_didPlay);
    EXPECT_FALSE(obs2.m_didPause);
    EXPECT_FALSE(obs2.m_didStop);
    
    timer.play();
    
    EXPECT_TRUE(obs1.m_willPlay);
    EXPECT_TRUE(obs1.m_didPlay);
    EXPECT_FALSE(obs1.m_didPause);
    EXPECT_FALSE(obs1.m_didStop);
    EXPECT_TRUE(obs2.m_willPlay);
    EXPECT_TRUE(obs2.m_didPlay);
    EXPECT_FALSE(obs2.m_didPause);
    EXPECT_FALSE(obs2.m_didStop);
    
    timer.pause();
    
    EXPECT_TRUE(obs1.m_willPlay);
    EXPECT_TRUE(obs1.m_didPlay);
    EXPECT_TRUE(obs1.m_didPause);
    EXPECT_FALSE(obs1.m_didStop);
    EXPECT_TRUE(obs2.m_willPlay);
    EXPECT_TRUE(obs2.m_didPlay);
    EXPECT_TRUE(obs2.m_didPause);
    EXPECT_FALSE(obs2.m_didStop);
    
    timer.stop();
    
    EXPECT_TRUE(obs1.m_willPlay);
    EXPECT_TRUE(obs1.m_didPlay);
    EXPECT_TRUE(obs1.m_didPause);
    EXPECT_TRUE(obs1.m_didStop);
    EXPECT_TRUE(obs2.m_willPlay);
    EXPECT_TRUE(obs2.m_didPlay);
    EXPECT_TRUE(obs2.m_didPause);
    EXPECT_TRUE(obs2.m_didStop);
}

TEST(TimerTest, Priorities)
{
    sfe::Timer timer;
    MyObserver obs1(2), obs2(3), obs3(1);
    
    timer.addObserver(obs1, 0);
    timer.addObserver(obs2, 2);
    timer.addObserver(obs3, -7);
    
    timer.play();
}
