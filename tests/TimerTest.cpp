
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE TimerTest
#include <boost/test/unit_test.hpp>
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
            BOOST_CHECK(m_didPlay == false);
            m_willPlay = true;
            
            if (m_expectedCallOrder > 0)
            {
                BOOST_CHECK(++globalCallOrder == m_expectedCallOrder);
            }
        }
        
        void didPlay(const sfe::Timer& timer, sfe::Status previousStatus)
        {
            BOOST_CHECK(m_willPlay == true);
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
    
BOOST_AUTO_TEST_CASE(TimerTestBase)
{
    sfe::Timer timer;
    
    BOOST_CHECK(timer.getOffset() == sf::Time::Zero);
    BOOST_CHECK(timer.getStatus() == sfe::Stopped);
    
    BOOST_CHECK_NO_THROW(timer.play());
    BOOST_CHECK(timer.getStatus() == sfe::Playing);
    BOOST_CHECK_THROW(timer.play(), std::runtime_error);
    
    BOOST_CHECK_NO_THROW(timer.pause());
    BOOST_CHECK(timer.getStatus() == sfe::Paused);
    BOOST_CHECK_THROW(timer.pause(), std::runtime_error);
    
    BOOST_CHECK_NO_THROW(timer.stop());
    BOOST_CHECK(timer.getStatus() == sfe::Stopped);
    BOOST_CHECK_THROW(timer.stop(), std::runtime_error);
    
    BOOST_CHECK(timer.getOffset() == sf::Time::Zero);
    
    timer.play();
    sf::sleep(sf::seconds(1));
    BOOST_CHECK(timer.getOffset() != sf::Time::Zero);
}

BOOST_AUTO_TEST_CASE(TimerTestNotifications)
{
    sfe::Timer timer;
    MyObserver obs1, obs2;
    
    timer.addObserver(obs1);
    timer.addObserver(obs2);
    
    BOOST_CHECK(obs1.m_willPlay == false);
    BOOST_CHECK(obs1.m_didPlay == false);
    BOOST_CHECK(obs1.m_didPause == false);
    BOOST_CHECK(obs1.m_didStop == false);
    BOOST_CHECK(obs2.m_willPlay == false);
    BOOST_CHECK(obs2.m_didPlay == false);
    BOOST_CHECK(obs2.m_didPause == false);
    BOOST_CHECK(obs2.m_didStop == false);
    
    timer.play();
    
    BOOST_CHECK(obs1.m_willPlay == true);
    BOOST_CHECK(obs1.m_didPlay == true);
    BOOST_CHECK(obs1.m_didPause == false);
    BOOST_CHECK(obs1.m_didStop == false);
    BOOST_CHECK(obs2.m_willPlay == true);
    BOOST_CHECK(obs2.m_didPlay == true);
    BOOST_CHECK(obs2.m_didPause == false);
    BOOST_CHECK(obs2.m_didStop == false);
    
    timer.pause();
    
    BOOST_CHECK(obs1.m_willPlay == true);
    BOOST_CHECK(obs1.m_didPlay == true);
    BOOST_CHECK(obs1.m_didPause == true);
    BOOST_CHECK(obs1.m_didStop == false);
    BOOST_CHECK(obs2.m_willPlay == true);
    BOOST_CHECK(obs2.m_didPlay == true);
    BOOST_CHECK(obs2.m_didPause == true);
    BOOST_CHECK(obs2.m_didStop == false);
    
    timer.stop();
    
    BOOST_CHECK(obs1.m_willPlay == true);
    BOOST_CHECK(obs1.m_didPlay == true);
    BOOST_CHECK(obs1.m_didPause == true);
    BOOST_CHECK(obs1.m_didStop == true);
    BOOST_CHECK(obs2.m_willPlay == true);
    BOOST_CHECK(obs2.m_didPlay == true);
    BOOST_CHECK(obs2.m_didPause == true);
    BOOST_CHECK(obs2.m_didStop == true);
}

BOOST_AUTO_TEST_CASE(TimerTestPriorities)
{
    sfe::Timer timer;
    MyObserver obs1(2), obs2(3), obs3(1);
    
    timer.addObserver(obs1, 0);
    timer.addObserver(obs2, 2);
    timer.addObserver(obs3, -7);
    
    timer.play();
}
