#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE RangeBasedLoop
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_CASE(RangeBasedLoop)
{
	std::vector<int> vector;
    vector.push_back(10);
    for (int i : vector)
    {
        BOOST_CHECK(i == 10);
    }
}
