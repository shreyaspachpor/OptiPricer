#include <iostream>
#include "../include/optiverse/models.hpp"   

int main() {
    using namespace optiverse::models;

    // Parameters: strike K, volatility sigma, risk-free rate r, time T, underlying S
    BlackScholesModel bsm(100, 0.2, 0.05, 1, 100);

    // Calculate call and put prices
    double call = bsm.call_price();
    double put  = bsm.put_price();

    // Print the results
    std::cout << "Call Price: " << call << std::endl;
    std::cout << "Put Price : " << put << std::endl;

    return 0;
}
