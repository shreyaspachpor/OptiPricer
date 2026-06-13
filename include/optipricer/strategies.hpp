#ifndef OPTIPRICER_STRATEGIES_HPP
#define OPTIPRICER_STRATEGIES_HPP

#include "models.hpp"
#include "greeks.hpp"
#include <vector>
#include <string>
#include <stdexcept>

namespace optipricer {
namespace strategies {

enum class OptionType { CALL, PUT };
enum class PositionType { LONG, SHORT };

struct Position {
    OptionType option_type;
    PositionType position_type;
    double quantity;
    double strike;

    Position(OptionType opt_type, PositionType pos_type, double qty, double K)
        : option_type(opt_type), position_type(pos_type), quantity(qty), strike(K) {}
};

// --- Base OptionsStrategy Class ---
class OptionsStrategy {
protected:
    std::vector<Position> positions;
    double underlying_price;
    double volatility;
    double risk_free_rate;
    double time_to_maturity;
    double dividend_yield;
    std::string strategy_name;

    void validate_inputs() const {
        if (underlying_price <= 0.0) {
            throw std::invalid_argument("Underlying price must be positive, got: " + std::to_string(underlying_price));
        }
        if (volatility < 0.0) {
            throw std::invalid_argument("Volatility must be non-negative, got: " + std::to_string(volatility));
        }
        if (volatility > 10.0) {
            throw std::invalid_argument("Volatility seems unreasonably high (>1000%), got: " + std::to_string(volatility));
        }
        if (time_to_maturity <= 0.0) {
            throw std::invalid_argument("Time to maturity must be positive, got: " + std::to_string(time_to_maturity));
        }
        if (time_to_maturity > 100.0) {
            throw std::invalid_argument("Time to maturity seems unreasonably high (>100 years), got: " + std::to_string(time_to_maturity));
        }
        if (dividend_yield < 0.0) {
            throw std::invalid_argument("Dividend yield must be non-negative, got: " + std::to_string(dividend_yield));
        }
        if (dividend_yield > 10.0) {
            throw std::invalid_argument("Dividend yield seems unreasonably high (>1000%), got: " + std::to_string(dividend_yield));
        }
        if (std::isnan(underlying_price) || std::isnan(volatility) || std::isnan(risk_free_rate) || std::isnan(time_to_maturity) || std::isnan(dividend_yield)) {
            throw std::invalid_argument("Input parameters cannot be NaN");
        }
        if (std::isinf(underlying_price) || std::isinf(volatility) || std::isinf(risk_free_rate) || std::isinf(time_to_maturity) || std::isinf(dividend_yield)) {
            throw std::invalid_argument("Input parameters cannot be infinite");
        }
    }

public:
    OptionsStrategy(double S, double sigma, double r, double T, const std::string& name, double q = 0.0)
        : underlying_price(S), volatility(sigma), risk_free_rate(r),
          time_to_maturity(T), dividend_yield(q), strategy_name(name) {
        validate_inputs();
    }

    virtual ~OptionsStrategy() = default;


    void add_position(OptionType opt_type, PositionType pos_type, double qty, double strike) {
        if (strike <= 0.0) {
            throw std::invalid_argument("Strike price must be positive, got: " + std::to_string(strike));
        }
        if (qty <= 0.0) {
            throw std::invalid_argument("Quantity must be positive, got: " + std::to_string(qty));
        }
        if (std::isnan(strike) || std::isnan(qty)) {
            throw std::invalid_argument("Position parameters cannot be NaN");
        }
        if (std::isinf(strike) || std::isinf(qty)) {
            throw std::invalid_argument("Position parameters cannot be infinite");
        }
        positions.emplace_back(opt_type, pos_type, qty, strike);
    }


    double total_value() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            double value = (pos.option_type == OptionType::CALL) 
                         ? model.call_price() 
                         : model.put_price();
            total += (pos.position_type == PositionType::LONG) ? value * pos.quantity
                                                               : -value * pos.quantity;
        }
        return total;
    }

    // Total Greeks (Delta example, similar for Gamma, Vega, Theta)
    double total_delta() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double delta = (pos.option_type == OptionType::CALL) ? greeks.call_delta() : greeks.put_delta();
            total += (pos.position_type == PositionType::LONG) ? delta * pos.quantity : -delta * pos.quantity;
        }
        return total;
    }

    double total_gamma() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double g = greeks.gamma();
            total += (pos.position_type == PositionType::LONG) ? g * pos.quantity : -g * pos.quantity;
        }
        return total;
    }

    double total_vega() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double v = greeks.vega();
            total += (pos.position_type == PositionType::LONG) ? v * pos.quantity : -v * pos.quantity;
        }
        return total;
    }

    double total_theta() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double theta = (pos.option_type == OptionType::CALL) ? greeks.call_theta() : greeks.put_theta();
            total += (pos.position_type == PositionType::LONG) ? theta * pos.quantity : -theta * pos.quantity;
        }
        return total;
    }

    double total_rho() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double rho = (pos.option_type == OptionType::CALL) ? greeks.call_rho() : greeks.put_rho();
            total += (pos.position_type == PositionType::LONG) ? rho * pos.quantity : -rho * pos.quantity;
        }
        return total;
    }

    double total_vanna() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double v = greeks.vanna();
            total += (pos.position_type == PositionType::LONG) ? v * pos.quantity : -v * pos.quantity;
        }
        return total;
    }

    double total_volga() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double v = greeks.volga();
            total += (pos.position_type == PositionType::LONG) ? v * pos.quantity : -v * pos.quantity;
        }
        return total;
    }

    double total_charm() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price, dividend_yield);
            models::GreeksCalculator greeks(model);
            double c = (pos.option_type == OptionType::CALL) ? greeks.call_charm() : greeks.put_charm();
            total += (pos.position_type == PositionType::LONG) ? c * pos.quantity : -c * pos.quantity;
        }
        return total;
    }

    // Payoff at expiration
    double payoff_at_expiration(double S_T) const {
        if (S_T < 0.0) {
            throw std::invalid_argument("Stock price at expiration must be non-negative, got: " + std::to_string(S_T));
        }
        if (std::isnan(S_T)) {
            throw std::invalid_argument("Stock price at expiration cannot be NaN");
        }
        if (std::isinf(S_T)) {
            throw std::invalid_argument("Stock price at expiration cannot be infinite");
        }
        double total = 0.0;
        for (const auto& pos : positions) {
            double intrinsic = (pos.option_type == OptionType::CALL) 
                             ? std::max(S_T - pos.strike, 0.0) 
                             : std::max(pos.strike - S_T, 0.0);
            total += (pos.position_type == PositionType::LONG) ? intrinsic * pos.quantity
                                                               : -intrinsic * pos.quantity;
        }
        return total;
    }

    const std::vector<Position>& get_positions() const { return positions; }
    const std::string& get_name() const { return strategy_name; }
    double get_dividend_yield() const { return dividend_yield; }
};

// --- Atomic Option Classes ---
class LongCall : public OptionsStrategy {
public:
    LongCall(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Long Call", q) {
        add_position(OptionType::CALL, PositionType::LONG, qty, K);
    }
};

class ShortCall : public OptionsStrategy {
public:
    ShortCall(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Short Call", q) {
        add_position(OptionType::CALL, PositionType::SHORT, qty, K);
    }
};

class LongPut : public OptionsStrategy {
public:
    LongPut(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Long Put", q) {
        add_position(OptionType::PUT, PositionType::LONG, qty, K);
    }
};

class ShortPut : public OptionsStrategy {
public:
    ShortPut(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Short Put", q) {
        add_position(OptionType::PUT, PositionType::SHORT, qty, K);
    }
};


// --- Composed Strategies ---
class LongStraddle : public OptionsStrategy {
public:
    LongStraddle(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Long Straddle", q) {
        add_position(OptionType::CALL, PositionType::LONG, qty, K);
        add_position(OptionType::PUT, PositionType::LONG, qty, K);
    }
};

class ShortStraddle : public OptionsStrategy {
public:
    ShortStraddle(double S, double sigma, double r, double T, double K, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Short Straddle", q) {
        add_position(OptionType::CALL, PositionType::SHORT, qty, K);
        add_position(OptionType::PUT, PositionType::SHORT, qty, K);
    }
};

class LongStrangle : public OptionsStrategy {
public:
    LongStrangle(double S, double sigma, double r, double T, double K_put, double K_call, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Long Strangle", q) {
        if (K_put >= K_call) throw std::invalid_argument("Put strike must be < Call strike");
        add_position(OptionType::PUT, PositionType::LONG, qty, K_put);
        add_position(OptionType::CALL, PositionType::LONG, qty, K_call);
    }
};

class ShortStrangle : public OptionsStrategy {
public:
    ShortStrangle(double S, double sigma, double r, double T, double K_put, double K_call, double qty = 1.0, double q = 0.0)
        : OptionsStrategy(S, sigma, r, T, "Short Strangle", q) {
        if (K_put >= K_call) throw std::invalid_argument("Put strike must be < Call strike");
        add_position(OptionType::PUT, PositionType::SHORT, qty, K_put);
        add_position(OptionType::CALL, PositionType::SHORT, qty, K_call);
    }
};

} // namespace strategies
} // namespace optipricer

#endif // OPTIPRICER_STRATEGIES_HPP
