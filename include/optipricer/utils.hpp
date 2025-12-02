#ifndef OPTIPRICER_UTILS_HPP
#define OPTIPRICER_UTILS_HPP

#include <cmath>

namespace optipricer
{
    namespace utils
    {

        // Mathematical constants
        constexpr double DAYS_PER_YEAR = 365.0;
        constexpr double PERCENTAGE_DIVISOR = 100.0;
        constexpr double SQRT_2PI = 2.506628274631000502415765284811;

        /**
         * @brief Standard normal cumulative distribution function (CDF)
         * @param x The value at which to evaluate the CDF
         * @return The probability that a standard normal random variable is less than or equal to x
         */
        inline double norm_cdf(double x)
        {
            return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
        }

        /**
         * @brief Standard normal probability density function (PDF)
         * @param x The value at which to evaluate the PDF
         * @return The probability density at x for a standard normal distribution
         */
        inline double norm_pdf(double x)
        {
            return (1.0 / SQRT_2PI) * std::exp(-0.5 * x * x);
        }

    } // namespace utils
} // namespace optipricer

#endif // OPTIPRICER_UTILS_HPP
