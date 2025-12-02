#ifndef OPTIPRICER_GREEKS_CALCULATOR_HPP
#define OPTIPRICER_GREEKS_CALCULATOR_HPP
#include "models.hpp"
#include "utils.hpp"
#include <cmath>

namespace optipricer
{
    namespace models
    {

        /**
         * @brief Calculator for option Greeks (sensitivity measures)
         *
         * This class computes the Greeks (Delta, Gamma, Vega, Theta, Rho) for options
         * using a Black-Scholes model. The model is stored by value to ensure thread safety
         * and avoid lifetime issues.
         */
        class GreeksCalculator
        {
        private:
            BlackScholesModel model; // Stored by value to avoid dangling reference issues

        public:
            explicit GreeksCalculator(const BlackScholesModel &bs_model)
                : model(bs_model) {}

            double call_delta() const
            {
                return utils::norm_cdf(model.d1());
            }

            double put_delta() const
            {
                return utils::norm_cdf(model.d1()) - 1.0;
            }

            double gamma() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();

                return utils::norm_pdf(model.d1()) / (S * sigma * std::sqrt(T));
            }

            double vega() const
            {
                double S = model.get_underlying_price();
                double T = model.get_time_to_maturity();

                // Vega is divided by 100 to express per 1% change in volatility
                return S * utils::norm_pdf(model.d1()) * std::sqrt(T) / utils::PERCENTAGE_DIVISOR;
            }

            double call_theta() const
            {
                double S = model.get_underlying_price();
                double K = model.get_strike_price();
                double r = model.get_risk_free_rate();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();

                double term1 = -(S * utils::norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
                double term2 = -r * K * std::exp(-r * T) * utils::norm_cdf(model.d2());

                // Theta is divided by 365 to express per-day time decay
                return (term1 + term2) / utils::DAYS_PER_YEAR;
            }

            double put_theta() const
            {
                double S = model.get_underlying_price();
                double K = model.get_strike_price();
                double r = model.get_risk_free_rate();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();

                double term1 = -(S * utils::norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
                double term2 = r * K * std::exp(-r * T) * utils::norm_cdf(-model.d2());

                // Theta is divided by 365 to express per-day time decay
                return (term1 + term2) / utils::DAYS_PER_YEAR;
            }

            double call_rho() const
            {
                double K = model.get_strike_price();
                double r = model.get_risk_free_rate();
                double T = model.get_time_to_maturity();

                // Rho is divided by 100 to express per 1% change in interest rate
                return K * T * std::exp(-r * T) * utils::norm_cdf(model.d2()) / utils::PERCENTAGE_DIVISOR;
            }

            double put_rho() const
            {
                double K = model.get_strike_price();
                double r = model.get_risk_free_rate();
                double T = model.get_time_to_maturity();

                // Rho is divided by 100 to express per 1% change in interest rate
                return -K * T * std::exp(-r * T) * utils::norm_cdf(-model.d2()) / utils::PERCENTAGE_DIVISOR;
            }
        };

    }
}

#endif // OPTIPRICER_GREEKS_CALCULATOR_HPP