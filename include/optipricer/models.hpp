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
            double dividend_yield;

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
                if (dividend_yield < 0.0)
                {
                    throw std::invalid_argument("Dividend yield must be non-negative, got: " + std::to_string(dividend_yield));
                }
                if (dividend_yield > 10.0)
                {
                    throw std::invalid_argument("Dividend yield seems unreasonably high (>1000%), got: " + std::to_string(dividend_yield));
                }
                if (std::isnan(strike_price) || std::isnan(volatility) || std::isnan(risk_free_rate) ||
                    std::isnan(time_to_maturity) || std::isnan(underlying_price) || std::isnan(dividend_yield))
                {
                    throw std::invalid_argument("Input parameters cannot be NaN");
                }
                if (std::isinf(strike_price) || std::isinf(volatility) || std::isinf(risk_free_rate) ||
                    std::isinf(time_to_maturity) || std::isinf(underlying_price) || std::isinf(dividend_yield))
                {
                    throw std::invalid_argument("Input parameters cannot be infinite");
                }
            }

        public:
            BlackScholesModel(double K, double sigma, double r, double T, double S, double q = 0.0)
                : strike_price(K), volatility(sigma), risk_free_rate(r),
                  time_to_maturity(T), underlying_price(S), dividend_yield(q)
            {
                validate_inputs();
            }

            double d1() const
            {
                // Handle edge case where volatility is very small or time to maturity is very small
                if (volatility < 1e-10 || time_to_maturity < 1e-10)
                {
                    double discounted_strike = strike_price * std::exp(-risk_free_rate * time_to_maturity);
                    double discounted_underlying = underlying_price * std::exp(-dividend_yield * time_to_maturity);
                    if (discounted_underlying > discounted_strike)
                    {
                        return 1e15; // Represents +infinity
                    }
                    else if (discounted_underlying < discounted_strike)
                    {
                        return -1e15; // Represents -infinity
                    }
                    else
                    {
                        return 0.0;
                    }
                }

                double sqrt_T = std::sqrt(time_to_maturity);
                double vol_sqrt_T = volatility * sqrt_T;

                return (std::log(underlying_price / strike_price) +
                        (risk_free_rate - dividend_yield + 0.5 * volatility * volatility) * time_to_maturity) /
                       vol_sqrt_T;
            }

            double d2() const
            {
                if (volatility < 1e-10 || time_to_maturity < 1e-10)
                {
                    return d1();
                }
                return d1() - volatility * std::sqrt(time_to_maturity);
            }

            double call_price() const
            {
                try
                {
                    double D1 = d1();
                    double D2 = d2();
                    double discount_factor = std::exp(-risk_free_rate * time_to_maturity);
                    double div_discount = std::exp(-dividend_yield * time_to_maturity);

                    // Check for overflow/underflow
                    if (std::isinf(discount_factor) || std::isnan(discount_factor) ||
                        std::isinf(div_discount) || std::isnan(div_discount))
                    {
                        throw std::runtime_error("Discount factor calculation resulted in invalid value");
                    }

                    return underlying_price * div_discount * utils::norm_cdf(D1) -
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
                    double div_discount = std::exp(-dividend_yield * time_to_maturity);

                    // Check for overflow/underflow
                    if (std::isinf(discount_factor) || std::isnan(discount_factor) ||
                        std::isinf(div_discount) || std::isnan(div_discount))
                    {
                        throw std::runtime_error("Discount factor calculation resulted in invalid value");
                    }

                    return strike_price * discount_factor * utils::norm_cdf(-D2) -
                           underlying_price * div_discount * utils::norm_cdf(-D1);
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
            double get_dividend_yield() const { return dividend_yield; }
        };

        /**
         * @brief Calculates the implied volatility for an option.
         * 
         * Uses a hybrid Newton-Raphson and Bisection solver.
         */
        inline double calculate_implied_volatility(
            double market_price,
            double strike_price,
            double risk_free_rate,
            double time_to_maturity,
            double underlying_price,
            double dividend_yield,
            bool is_call,
            double tol = 1e-6,
            int max_iter = 100)
        {
            if (market_price <= 0.0)
            {
                throw std::invalid_argument("Market price must be positive, got: " + std::to_string(market_price));
            }
            if (strike_price <= 0.0)
            {
                throw std::invalid_argument("Strike price must be positive, got: " + std::to_string(strike_price));
            }
            if (time_to_maturity <= 0.0)
            {
                throw std::invalid_argument("Time to maturity must be positive, got: " + std::to_string(time_to_maturity));
            }
            if (underlying_price <= 0.0)
            {
                throw std::invalid_argument("Underlying price must be positive, got: " + std::to_string(underlying_price));
            }
            if (dividend_yield < 0.0)
            {
                throw std::invalid_argument("Dividend yield must be non-negative, got: " + std::to_string(dividend_yield));
            }

            double discount_factor = std::exp(-risk_free_rate * time_to_maturity);
            double div_discount = std::exp(-dividend_yield * time_to_maturity);

            // Theoretical bounds
            double min_price = 0.0;
            double max_price = 0.0;
            if (is_call)
            {
                min_price = std::max(underlying_price * div_discount - strike_price * discount_factor, 0.0);
                max_price = underlying_price * div_discount;
            }
            else
            {
                min_price = std::max(strike_price * discount_factor - underlying_price * div_discount, 0.0);
                max_price = strike_price * discount_factor;
            }

            if (market_price < min_price || market_price > max_price)
            {
                throw std::invalid_argument("Market price " + std::to_string(market_price) + 
                                            " is outside theoretical Black-Scholes bounds [" + 
                                            std::to_string(min_price) + ", " + std::to_string(max_price) + "]");
            }

            // Bracket the root
            double low = 1e-10;
            double high = 10.0;
            
            // Check if price at high bracket is too small (volatility extremely high, expanding search)
            {
                BlackScholesModel test_high(strike_price, high, risk_free_rate, time_to_maturity, underlying_price, dividend_yield);
                double price_high = is_call ? test_high.call_price() : test_high.put_price();
                while (price_high < market_price && high < 50.0)
                {
                    high += 5.0;
                    BlackScholesModel test_high_new(strike_price, high, risk_free_rate, time_to_maturity, underlying_price, dividend_yield);
                    price_high = is_call ? test_high_new.call_price() : test_high_new.put_price();
                }
                if (price_high < market_price)
                {
                    throw std::invalid_argument("Market price is too high for maximum supported volatility.");
                }
            }

            // Newton-Raphson Solver with Bisection Fallback
            double sigma = 0.5; // Initial guess
            for (int i = 0; i < max_iter; ++i)
            {
                BlackScholesModel model(strike_price, sigma, risk_free_rate, time_to_maturity, underlying_price, dividend_yield);
                double price = is_call ? model.call_price() : model.put_price();
                double diff = price - market_price;

                if (std::abs(diff) < tol)
                {
                    return sigma;
                }

                // Compute exact analytical vega to use as derivative
                double d1_val = model.d1();
                double vega_derivative = underlying_price * div_discount * utils::norm_pdf(d1_val) * std::sqrt(time_to_maturity);

                double sigma_new = -1.0;
                if (vega_derivative > 1e-8)
                {
                    sigma_new = sigma - diff / vega_derivative;
                }

                // If Newton step went out of bounds or vega is too small, use bisection
                if (sigma_new <= low || sigma_new >= high)
                {
                    if (diff > 0.0)
                    {
                        high = sigma;
                    }
                    else
                    {
                        low = sigma;
                    }
                    sigma = 0.5 * (low + high);
                }
                else
                {
                    // Update brackets and apply Newton-Raphson update
                    if (diff > 0.0)
                    {
                        high = sigma;
                    }
                    else
                    {
                        low = sigma;
                    }
                    sigma = sigma_new;
                }

                // Check bracket size for convergence
                if (std::abs(high - low) < tol)
                {
                    return 0.5 * (low + high);
                }
            }

            return sigma;
        }
    }
}

#endif // OPTIPRICER_MODELS_HPP