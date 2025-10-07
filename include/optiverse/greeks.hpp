#ifndef OPTIVERSE_GREEKS_CALCULATOR_HPP
#define OPTIVERSE_GREEKS_CALCULATOR_HPP
#include "models.hpp"
#include <cmath>

namespace optiverse {
    inline double norm_pdf(double x) {
    return (1.0 / std::sqrt(2.0 * M_PI)) * std::exp(-0.5 * x * x);
}

namespace models {

class GreeksCalculator {
private:
    const BlackScholesModel& model;

public:
    explicit GreeksCalculator(const BlackScholesModel& bs_model) 
        : model(bs_model) {}

    double call_delta() const {
        return norm_cdf(model.d1());
    }

    double put_delta() const {
        return norm_cdf(model.d1()) - 1.0;
    }

    double gamma() const {
        double S = model.get_underlying_price();
        double sigma = model.get_volatility();
        double T = model.get_time_to_maturity();
        
        return norm_pdf(model.d1()) / (S * sigma * std::sqrt(T));
    }

    double vega() const {
        double S = model.get_underlying_price();
        double T = model.get_time_to_maturity();
        
        return S * norm_pdf(model.d1()) * std::sqrt(T) / 100.0;
    }


    double call_theta() const {
        double S = model.get_underlying_price();
        double K = model.get_strike_price();
        double r = model.get_risk_free_rate();
        double sigma = model.get_volatility();
        double T = model.get_time_to_maturity();
        
        double term1 = -(S * norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
        double term2 = -r * K * std::exp(-r * T) * norm_cdf(model.d2());
        
        return (term1 + term2) / 365.0;
    }


    double put_theta() const {
        double S = model.get_underlying_price();
        double K = model.get_strike_price();
        double r = model.get_risk_free_rate();
        double sigma = model.get_volatility();
        double T = model.get_time_to_maturity();
        
        double term1 = -(S * norm_pdf(model.d1()) * sigma) / (2.0 * std::sqrt(T));
        double term2 = r * K * std::exp(-r * T) * norm_cdf(-model.d2());
        
        return (term1 + term2) / 365.0;
    }

    double call_rho() const {
        double K = model.get_strike_price();
        double r = model.get_risk_free_rate();
        double T = model.get_time_to_maturity();
        
        return K * T * std::exp(-r * T) * norm_cdf(model.d2()) / 100.0;
    }

    double put_rho() const {
        double K = model.get_strike_price();
        double r = model.get_risk_free_rate();
        double T = model.get_time_to_maturity();
        
        return -K * T * std::exp(-r * T) * norm_cdf(-model.d2()) / 100.0;
    }

};

}
}

#endif // OPTIVERSE_GREEKS_CALCULATOR_HPP