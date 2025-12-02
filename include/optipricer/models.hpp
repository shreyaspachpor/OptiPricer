#ifndef OPTIPRICER_MODELS_HPP
#define OPTIPRICER_MODELS_HPP

#include <cmath>
#include <stdexcept>
#include <string>
#include <limits>
#include "utils.hpp"

namespace optipricer
{
    namespace models
    {
        class BlackScholesModel
        {
        private:
            double strike_price;
            double volatility;
            double risk_free_rate;
            double time_to_maturity;
            double underlying_price;

            void validate_inputs() const
            {
                if (strike_price <= 0.0)
                {
                    throw std::invalid_argument("Strike price must be positive, got: " + std::to_string(strike_price));
                }
                if (volatility < 0.0)
                {
                    throw std::invalid_argument("Volatility must be non-negative, got: " + std::to_string(volatility));
                }
                if (volatility > 10.0)
                {
                    throw std::invalid_argument("Volatility seems unreasonably high (>1000%), got: " + std::to_string(volatility));
                }
                if (time_to_maturity <= 0.0)
                {
                    throw std::invalid_argument("Time to maturity must be positive, got: " + std::to_string(time_to_maturity));
                }
                if (time_to_maturity > 100.0)
                {
                    throw std::invalid_argument("Time to maturity seems unreasonably high (>100 years), got: " + std::to_string(time_to_maturity));
                }
                if (underlying_price <= 0.0)
                {
                    throw std::invalid_argument("Underlying price must be positive, got: " + std::to_string(underlying_price));
                }
                if (std::isnan(strike_price) || std::isnan(volatility) || std::isnan(risk_free_rate) ||
                    std::isnan(time_to_maturity) || std::isnan(underlying_price))
                {
                    throw std::invalid_argument("Input parameters cannot be NaN");
                }
                if (std::isinf(strike_price) || std::isinf(volatility) || std::isinf(risk_free_rate) ||
                    std::isinf(time_to_maturity) || std::isinf(underlying_price))
                {
                    throw std::invalid_argument("Input parameters cannot be infinite");
                }
            }

        public:
            BlackScholesModel(double K, double sigma, double r, double T, double S)
                : strike_price(K), volatility(sigma), risk_free_rate(r),
                  time_to_maturity(T), underlying_price(S)
            {
                validate_inputs();
            }

            double d1() const
            {
                // Handle edge case where volatility is very small
                if (volatility < 1e-10 || time_to_maturity < 1e-10)
                {
                    // When vol or time approaches zero, option becomes intrinsic value
                    throw std::runtime_error("Volatility or time to maturity too small for accurate Black-Scholes calculation");
                }

                double sqrt_T = std::sqrt(time_to_maturity);
                double vol_sqrt_T = volatility * sqrt_T;

                return (std::log(underlying_price / strike_price) +
                        (risk_free_rate + 0.5 * volatility * volatility) * time_to_maturity) /
                       vol_sqrt_T;
            }

            double d2() const
            {
                return d1() - volatility * std::sqrt(time_to_maturity);
            }

            double call_price() const
            {
                try
                {
                    double D1 = d1();
                    double D2 = d2();
                    double discount_factor = std::exp(-risk_free_rate * time_to_maturity);

                    // Check for overflow/underflow
                    if (std::isinf(discount_factor) || std::isnan(discount_factor))
                    {
                        throw std::runtime_error("Discount factor calculation resulted in invalid value");
                    }

                    return underlying_price * utils::norm_cdf(D1) -
                           strike_price * discount_factor * utils::norm_cdf(D2);
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error("Error calculating call price: " + std::string(e.what()));
                }
            }

            double put_price() const
            {
                try
                {
                    double D1 = d1();
                    double D2 = d2();
                    double discount_factor = std::exp(-risk_free_rate * time_to_maturity);

                    // Check for overflow/underflow
                    if (std::isinf(discount_factor) || std::isnan(discount_factor))
                    {
                        throw std::runtime_error("Discount factor calculation resulted in invalid value");
                    }

                    return strike_price * discount_factor * utils::norm_cdf(-D2) -
                           underlying_price * utils::norm_cdf(-D1);
                }
                catch (const std::exception &e)
                {
                    throw std::runtime_error("Error calculating put price: " + std::string(e.what()));
                }
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

#endif // OPTIPRICER_MODELS_HPP