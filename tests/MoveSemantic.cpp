#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE MoveSemantic
#include <boost/test/unit_test.hpp>
#include <algorithm>    // std::move (ranges)
#include <utility>      // std::move (objects)
#include <iostream>

BOOST_AUTO_TEST_CASE(MoveSemantic_StdMove)
{
    std::vector<std::string> foo = {"air", "water", "fire", "earth"};
    std::vector<std::string> bar (4);
    
    // moving ranges:
    std::move(foo.begin(), foo.begin()+4, bar.begin());
    
    // moving container:
    foo = std::move(bar);
}

// Try with our own move-compliant object
struct MoveableObject {
    MoveableObject(int initValue)
    {
        complexType = {initValue, initValue, initValue};
    }
    
    MoveableObject(const MoveableObject& other)
    {
        throw std::runtime_error("I want to move, not copy!");
    }
    
    MoveableObject(MoveableObject&& other)
    {
        complexType = std::move(other.complexType);
        other.complexType.clear();
        
        std::cout << "moving throught constructor" << std::endl;
    }
    
    MoveableObject& operator=(MoveableObject&& other)
    {
        if (this != &other)
        {
            complexType = std::move(other.complexType);
            other.complexType.clear();
        }
        
        std::cout << "moving throught operator=" << std::endl;
        return *this;
    }
    
    std::vector<int> complexType;
};

BOOST_AUTO_TEST_CASE(MoveSemantic_MoveableObject)
{
    MoveableObject firstObject(37);
    BOOST_CHECK(firstObject.complexType[0] == 37);
    
    MoveableObject secondObject(422);
    BOOST_CHECK(secondObject.complexType[0] == 422);
    BOOST_CHECK(secondObject.complexType[0] != firstObject.complexType[0]);
    
    secondObject = std::move(firstObject);
    
    BOOST_CHECK(firstObject.complexType.size() == 0);
    BOOST_CHECK(secondObject.complexType[0] == 37);
}
