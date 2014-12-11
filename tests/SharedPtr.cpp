
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE SharedPtr

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(SharedPtr)
{
    std::shared_ptr<int> ptr(new int);
    BOOST_CHECK(ptr);
}
