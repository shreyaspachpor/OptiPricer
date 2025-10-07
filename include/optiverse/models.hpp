#ifndef OPTIVERSE_MODELS_HPP
#define OPTIVERSE_MODELS_HPP

#include <cmath>

namespace optiverse {
    inline double norm_cdf(double x) {
        return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
    }
    
    namespace models {
        class BlackScholesModel {
        private:
            double strike_price;
            double volatility;
            double risk_free_rate;
            double time_to_maturity;
            double underlying_price;

        public:
            BlackScholesModel(double K, double sigma, double r, double T, double S)
                : strike_price(K), volatility(sigma), risk_free_rate(r), 
                  time_to_maturity(T), underlying_price(S) {}

            double d1() const {
                return (std::log(underlying_price / strike_price) + 
                       (risk_free_rate + 0.5 * volatility * volatility) * time_to_maturity) /
                       (volatility * std::sqrt(time_to_maturity));
            }

            double d2() const {
                return d1() - volatility * std::sqrt(time_to_maturity);
            }

            double call_price() const {
                double D1 = d1();
                double D2 = d2();
                return underlying_price * norm_cdf(D1) - 
                       strike_price * std::exp(-risk_free_rate * time_to_maturity) * norm_cdf(D2);
            }

            double put_price() const {
                double D1 = d1();
                double D2 = d2();
                return strike_price * std::exp(-risk_free_rate * time_to_maturity) * norm_cdf(-D2) - 
                       underlying_price * norm_cdf(-D1);
            }

            // Getters for model parameters
            double get_strike_price() const { return strike_price; }
            double get_volatility() const { return volatility; }
            double get_risk_free_rate() const { return risk_free_rate; }
            double get_time_to_maturity() const { return time_to_maturity; }
            double get_underlying_price() const { return underlying_price; }

            
        };
    }
}

#endif // OPTIVERSE_MODELS_HPP