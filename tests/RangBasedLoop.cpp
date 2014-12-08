#include <vector>
#include <iostream>
int main(int argc, char** argv)
{
    std::vector<int> vector;
    vector.push_back(10);
    for (int i : vector)
    {
        std::cout << i << std::endl;
    }
}