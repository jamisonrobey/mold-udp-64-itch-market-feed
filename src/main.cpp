#include "Market_Feed.h"
#include <iostream>

int main()
{
    try
    {
        Market_Feed feed("239.0.0.1", 12345, 1, "../data/12302019.NASDAQ_ITCH50", std::chrono::microseconds(150));
        feed.start();
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
