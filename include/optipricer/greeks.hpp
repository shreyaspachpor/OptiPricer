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

            const BlackScholesModel& get_model() const { return model; }

            double call_delta() const
            {
                double q = model.get_dividend_yield();
                double T = model.get_time_to_maturity();
                return std::exp(-q * T) * utils::norm_cdf(model.d1());
            }

            double put_delta() const
            {
                double q = model.get_dividend_yield();
                double T = model.get_time_to_maturity();
                return std::exp(-q * T) * (utils::norm_cdf(model.d1()) - 1.0);
            }

            double gamma() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                return std::exp(-q * T) * utils::norm_pdf(model.d1()) / (S * sigma * std::sqrt(T));
            }

            double vega() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                // Vega is divided by 100 to express per 1% change in volatility
                return S * std::exp(-q * T) * utils::norm_pdf(model.d1()) * std::sqrt(T) / utils::PERCENTAGE_DIVISOR;
            }

            double call_theta() const
            {
                double S = model.get_underlying_price();
                double K = model.get_strike_price();
                double r = model.get_risk_free_rate();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    double discounted_strike = K * std::exp(-r * T);
                    double discounted_underlying = S * std::exp(-q * T);
                    double term2 = 0.0;
                    if (discounted_underlying > discounted_strike)
                    {
                        term2 = q * S * std::exp(-q * T) - r * K * std::exp(-r * T);
                    }
                    else if (discounted_underlying < discounted_strike)
                    {
                        term2 = 0.0;
                    }
                    else
                    {
                        term2 = 0.5 * (q * S * std::exp(-q * T) - r * K * std::exp(-r * T));
                    }
                    return term2 / utils::DAYS_PER_YEAR;
                }

                double term1 = -(S * std::exp(-q * T) * utils::norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
                double term2 = q * S * std::exp(-q * T) * utils::norm_cdf(model.d1()) - r * K * std::exp(-r * T) * utils::norm_cdf(model.d2());

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
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    double discounted_strike = K * std::exp(-r * T);
                    double discounted_underlying = S * std::exp(-q * T);
                    double term2 = 0.0;
                    if (discounted_underlying < discounted_strike)
                    {
                        term2 = -q * S * std::exp(-q * T) + r * K * std::exp(-r * T);
                    }
                    else if (discounted_underlying > discounted_strike)
                    {
                        term2 = 0.0;
                    }
                    else
                    {
                        term2 = 0.5 * (-q * S * std::exp(-q * T) + r * K * std::exp(-r * T));
                    }
                    return term2 / utils::DAYS_PER_YEAR;
                }

                double term1 = -(S * std::exp(-q * T) * utils::norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
                double term2 = -q * S * std::exp(-q * T) * utils::norm_cdf(-model.d1()) + r * K * std::exp(-r * T) * utils::norm_cdf(-model.d2());

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

            /**
             * @brief Vanna: ∂²V/∂S∂σ (cross-gamma between spot and vol)
             * 
             * Measures how delta changes with volatility. Critical for
             * volatility hedging and skew trading.
             * Formula: -e^{-qT} * N'(d1) * d2 / σ
             */
            double vanna() const
            {
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                double d1_val = model.d1();
                double d2_val = model.d2();
                return -std::exp(-q * T) * utils::norm_pdf(d1_val) * d2_val / sigma;
            }

            /**
             * @brief Volga (Vomma): ∂²V/∂σ² (second derivative of price w.r.t. volatility)
             * 
             * Measures convexity of option value with respect to volatility.
             * Important for managing vega risk in large portfolios.
             * Formula: Vega * d1 * d2 / σ  (where Vega is the raw, unscaled vega)
             */
            double volga() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                double d1_val = model.d1();
                double d2_val = model.d2();
                // Raw vega (unscaled) = S * e^{-qT} * N'(d1) * sqrt(T)
                double raw_vega = S * std::exp(-q * T) * utils::norm_pdf(d1_val) * std::sqrt(T);
                return raw_vega * d1_val * d2_val / sigma;
            }

            /**
             * @brief Charm (Delta Bleed): -∂²V/∂t∂S (rate of delta decay over time)
             * 
             * Measures how delta changes as time passes. Essential for
             * daily delta hedging adjustments.
             * Call Charm = -e^{-qT} * [N'(d1) * (2(r-q)T - d2*σ*√T) / (2T*σ*√T) + q*N(d1)]
             */
            double call_charm() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double r = model.get_risk_free_rate();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                double d1_val = model.d1();
                double d2_val = model.d2();
                double sqrt_T = std::sqrt(T);
                double pdf_d1 = utils::norm_pdf(d1_val);

                double term1 = pdf_d1 * (2.0 * (r - q) * T - d2_val * sigma * sqrt_T) / (2.0 * T * sigma * sqrt_T);
                double term2 = -q * utils::norm_cdf(d1_val);

                // Charm = -dDelta/dT, expressed per day (divide by 365)
                return -std::exp(-q * T) * (term1 + term2) / utils::DAYS_PER_YEAR;
            }

            double put_charm() const
            {
                double S = model.get_underlying_price();
                double sigma = model.get_volatility();
                double T = model.get_time_to_maturity();
                double r = model.get_risk_free_rate();
                double q = model.get_dividend_yield();

                if (sigma < 1e-10 || T < 1e-10)
                {
                    return 0.0;
                }

                double d1_val = model.d1();
                double d2_val = model.d2();
                double sqrt_T = std::sqrt(T);
                double pdf_d1 = utils::norm_pdf(d1_val);

                double term1 = pdf_d1 * (2.0 * (r - q) * T - d2_val * sigma * sqrt_T) / (2.0 * T * sigma * sqrt_T);
                double term2 = q * utils::norm_cdf(-d1_val);

                return -std::exp(-q * T) * (term1 + term2) / utils::DAYS_PER_YEAR;
            }
        };

    }
}

#endif // OPTIPRICER_GREEKS_CALCULATOR_HPP