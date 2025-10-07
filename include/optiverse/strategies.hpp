#ifndef OPTIVERSE_STRATEGIES_HPP
#define OPTIVERSE_STRATEGIES_HPP

#include "models.hpp"
#include "greeks.hpp"
#include <vector>
#include <string>
#include <stdexcept>

namespace optiverse {
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
    std::string strategy_name;

public:
    OptionsStrategy(double S, double sigma, double r, double T, const std::string& name)
        : underlying_price(S), volatility(sigma), risk_free_rate(r),
          time_to_maturity(T), strategy_name(name) {}

    virtual ~OptionsStrategy() = default;


    void add_position(OptionType opt_type, PositionType pos_type, double qty, double strike) {
        positions.emplace_back(opt_type, pos_type, qty, strike);
    }


    double total_value() const {
        double total = 0.0;
        for (const auto& pos : positions) {
            models::BlackScholesModel model(pos.strike, volatility, risk_free_rate,
                                           time_to_maturity, underlying_price);
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
                                           time_to_maturity, underlying_price);
            models::GreeksCalculator greeks(model);
            double delta = (pos.option_type == OptionType::CALL) ? greeks.call_delta() : greeks.put_delta();
            total += (pos.position_type == PositionType::LONG) ? delta * pos.quantity : -delta * pos.quantity;
        }
        return total;
    }

    // Payoff at expiration
    double payoff_at_expiration(double S_T) const {
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
};

// --- Atomic Option Classes ---
class LongCall : public OptionsStrategy {
public:
    LongCall(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Long Call") {
        add_position(OptionType::CALL, PositionType::LONG, qty, K);
    }
};

class ShortCall : public OptionsStrategy {
public:
    ShortCall(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Short Call") {
        add_position(OptionType::CALL, PositionType::SHORT, qty, K);
    }
};

class LongPut : public OptionsStrategy {
public:
    LongPut(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Long Put") {
        add_position(OptionType::PUT, PositionType::LONG, qty, K);
    }
};

class ShortPut : public OptionsStrategy {
public:
    ShortPut(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Short Put") {
        add_position(OptionType::PUT, PositionType::SHORT, qty, K);
    }
};


// --- Composed Strategies ---
class LongStraddle : public OptionsStrategy {
public:
    LongStraddle(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Long Straddle") {
        add_position(OptionType::CALL, PositionType::LONG, qty, K);
        add_position(OptionType::PUT, PositionType::LONG, qty, K);
    }
};

class ShortStraddle : public OptionsStrategy {
public:
    ShortStraddle(double S, double sigma, double r, double T, double K, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Short Straddle") {
        add_position(OptionType::CALL, PositionType::SHORT, qty, K);
        add_position(OptionType::PUT, PositionType::SHORT, qty, K);
    }
};

class LongStrangle : public OptionsStrategy {
public:
    LongStrangle(double S, double sigma, double r, double T, double K_put, double K_call, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Long Strangle") {
        if (K_put >= K_call) throw std::invalid_argument("Put strike must be < Call strike");
        add_position(OptionType::PUT, PositionType::LONG, qty, K_put);
        add_position(OptionType::CALL, PositionType::LONG, qty, K_call);
    }
};

class ShortStrangle : public OptionsStrategy {
public:
    ShortStrangle(double S, double sigma, double r, double T, double K_put, double K_call, double qty = 1.0)
        : OptionsStrategy(S, sigma, r, T, "Short Strangle") {
        if (K_put >= K_call) throw std::invalid_argument("Put strike must be < Call strike");
        add_position(OptionType::PUT, PositionType::SHORT, qty, K_put);
        add_position(OptionType::CALL, PositionType::SHORT, qty, K_call);
    }
};

} // namespace strategies
} // namespace optiverse

#endif // OPTIVERSE_STRATEGIES_HPP
