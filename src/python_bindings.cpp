#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "optipricer/models.hpp"
#include "optipricer/greeks.hpp"
#include "optipricer/strategies.hpp"
#include "optipricer/utils.hpp"
#include <sstream>
#include <iomanip>

namespace py = pybind11;

inline std::string format_double(double val, int precision = 4) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << val;
    std::string str = out.str();
    size_t dot = str.find('.');
    if (dot != std::string::npos) {
        while (str.back() == '0') {
            str.pop_back();
        }
        if (str.back() == '.') {
            str.pop_back();
        }
    }
    return str;
}

PYBIND11_MODULE(_core, m)
{
     m.doc() = "OptiPricer: A comprehensive options pricing and analysis library";

     py::register_exception_translator([](std::exception_ptr p)
                                       {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const std::invalid_argument &e) {
            PyErr_SetString(PyExc_ValueError, e.what());
        } catch (const std::runtime_error &e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        } });

     // Utility functions
     m.def("norm_cdf", &optipricer::utils::norm_cdf, "Standard normal cumulative distribution function",
           py::arg("x"));
     m.def("norm_pdf", &optipricer::utils::norm_pdf, "Standard normal probability density function",
           py::arg("x"));

     // Models submodule
     py::module_ models = m.def_submodule("models", "Options pricing models");

     py::class_<optipricer::models::BlackScholesModel>(models, "BlackScholesModel")
          .def(py::init<double, double, double, double, double, double>(),
               "Initialize Black-Scholes model\n\n"
               "Parameters:\n"
               "  strike_price: Option strike price (K) - must be positive\n"
               "  volatility: Annualized volatility (sigma) - must be non-negative\n"
               "  risk_free_rate: Risk-free interest rate (r) - annualized\n"
               "  time_to_maturity: Time to expiration (T) in years - must be positive\n"
               "  underlying_price: Current underlying asset price (S) - must be positive\n"
               "  dividend_yield: Annual dividend yield (q) - must be non-negative (defaults to 0.0)\n\n"
               "Raises:\n"
               "  ValueError: If any parameter is invalid (negative, zero, NaN, or infinite)",
               py::arg("strike_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("underlying_price"), py::arg("dividend_yield") = 0.0)
          .def("d1", &optipricer::models::BlackScholesModel::d1,
               "Calculate d1 parameter")
          .def("d2", &optipricer::models::BlackScholesModel::d2,
               "Calculate d2 parameter")
          .def("call_price", &optipricer::models::BlackScholesModel::call_price,
               "Calculate call option price")
          .def("put_price", &optipricer::models::BlackScholesModel::put_price,
               "Calculate put option price")
          .def("get_strike_price", &optipricer::models::BlackScholesModel::get_strike_price,
               "Get strike price")
          .def("get_volatility", &optipricer::models::BlackScholesModel::get_volatility,
               "Get volatility")
          .def("get_risk_free_rate", &optipricer::models::BlackScholesModel::get_risk_free_rate,
               "Get risk-free rate")
          .def("get_time_to_maturity", &optipricer::models::BlackScholesModel::get_time_to_maturity,
               "Get time to maturity")
          .def("get_underlying_price", &optipricer::models::BlackScholesModel::get_underlying_price,
               "Get underlying price")
          .def("get_dividend_yield", &optipricer::models::BlackScholesModel::get_dividend_yield,
               "Get dividend yield")
          .def("__repr__", [](const optipricer::models::BlackScholesModel &model) {
               return "BlackScholesModel(strike_price=" + format_double(model.get_strike_price()) +
                      ", volatility=" + format_double(model.get_volatility()) +
                      ", risk_free_rate=" + format_double(model.get_risk_free_rate()) +
                      ", time_to_maturity=" + format_double(model.get_time_to_maturity()) +
                      ", underlying_price=" + format_double(model.get_underlying_price()) +
                      ", dividend_yield=" + format_double(model.get_dividend_yield()) + ")";
          });

     models.def("calculate_implied_volatility", &optipricer::models::calculate_implied_volatility,
                "Calculate implied volatility for an option",
                py::arg("market_price"), py::arg("strike_price"), py::arg("risk_free_rate"),
                py::arg("time_to_maturity"), py::arg("underlying_price"), py::arg("dividend_yield") = 0.0,
                py::arg("is_call") = true, py::arg("tol") = 1e-6, py::arg("max_iter") = 100);

     py::class_<optipricer::models::GreeksCalculator>(models, "GreeksCalculator")
          .def(py::init<const optipricer::models::BlackScholesModel &>(),
               "Initialize Greeks calculator with Black-Scholes model",
               py::arg("model"))
          .def("call_delta", &optipricer::models::GreeksCalculator::call_delta,
               "Calculate call option delta")
          .def("put_delta", &optipricer::models::GreeksCalculator::put_delta,
               "Calculate put option delta")
          .def("gamma", &optipricer::models::GreeksCalculator::gamma,
               "Calculate gamma")
          .def("vega", &optipricer::models::GreeksCalculator::vega,
               "Calculate vega")
          .def("call_theta", &optipricer::models::GreeksCalculator::call_theta,
               "Calculate call option theta")
          .def("put_theta", &optipricer::models::GreeksCalculator::put_theta,
               "Calculate put option theta")
          .def("call_rho", &optipricer::models::GreeksCalculator::call_rho,
               "Calculate call option rho")
          .def("put_rho", &optipricer::models::GreeksCalculator::put_rho,
               "Calculate put option rho")
          .def("vanna", &optipricer::models::GreeksCalculator::vanna,
               "Calculate vanna (dDelta/dVol)")
          .def("volga", &optipricer::models::GreeksCalculator::volga,
               "Calculate volga/vomma (d²Price/dVol²)")
          .def("call_charm", &optipricer::models::GreeksCalculator::call_charm,
               "Calculate call charm (delta decay per day)")
          .def("put_charm", &optipricer::models::GreeksCalculator::put_charm,
               "Calculate put charm (delta decay per day)")
          .def("__repr__", [](const optipricer::models::GreeksCalculator &calc) {
               const auto& model = calc.get_model();
               return "GreeksCalculator(model=BlackScholesModel(S=" + format_double(model.get_underlying_price()) +
                      ", K=" + format_double(model.get_strike_price()) +
                      ", vol=" + format_double(model.get_volatility()) +
                      ", r=" + format_double(model.get_risk_free_rate()) +
                      ", T=" + format_double(model.get_time_to_maturity()) +
                      ", q=" + format_double(model.get_dividend_yield()) + "))";  
          });

     // Strategies submodule
     py::module_ strategies = m.def_submodule("strategies", "Options trading strategies");

     py::enum_<optipricer::strategies::OptionType>(strategies, "OptionType")
          .value("CALL", optipricer::strategies::OptionType::CALL)
          .value("PUT", optipricer::strategies::OptionType::PUT);

     py::enum_<optipricer::strategies::PositionType>(strategies, "PositionType")
          .value("LONG", optipricer::strategies::PositionType::LONG)
          .value("SHORT", optipricer::strategies::PositionType::SHORT);

     py::class_<optipricer::strategies::Position>(strategies, "Position")
          .def(py::init<optipricer::strategies::OptionType, optipricer::strategies::PositionType, double, double>(),
               py::arg("option_type"), py::arg("position_type"), py::arg("quantity"), py::arg("strike"))
          .def_readwrite("option_type", &optipricer::strategies::Position::option_type)
          .def_readwrite("position_type", &optipricer::strategies::Position::position_type)
          .def_readwrite("quantity", &optipricer::strategies::Position::quantity)
          .def_readwrite("strike", &optipricer::strategies::Position::strike)
          .def("__repr__", [](const optipricer::strategies::Position &p) {
               std::string o_type = (p.option_type == optipricer::strategies::OptionType::CALL) ? "OptionType.CALL" : "OptionType.PUT";
               std::string p_type = (p.position_type == optipricer::strategies::PositionType::LONG) ? "PositionType.LONG" : "PositionType.SHORT";
               return "Position(option_type=" + o_type +
                      ", position_type=" + p_type +
                      ", quantity=" + format_double(p.quantity) +
                      ", strike=" + format_double(p.strike) + ")";
          });

     py::class_<optipricer::strategies::OptionsStrategy>(strategies, "OptionsStrategy")
          .def(py::init<double, double, double, double, const std::string&, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strategy_name"), py::arg("dividend_yield") = 0.0)
          .def("add_position", &optipricer::strategies::OptionsStrategy::add_position,
               "Add a position to the strategy",
               py::arg("option_type"), py::arg("position_type"), py::arg("quantity"), py::arg("strike"))
          .def("total_value", &optipricer::strategies::OptionsStrategy::total_value,
               "Calculate total strategy value")
          .def("total_delta", &optipricer::strategies::OptionsStrategy::total_delta,
               "Calculate total strategy delta")
          .def("total_gamma", &optipricer::strategies::OptionsStrategy::total_gamma,
               "Calculate total strategy gamma")
          .def("total_vega", &optipricer::strategies::OptionsStrategy::total_vega,
               "Calculate total strategy vega")
          .def("total_theta", &optipricer::strategies::OptionsStrategy::total_theta,
               "Calculate total strategy theta")
          .def("total_rho", &optipricer::strategies::OptionsStrategy::total_rho,
               "Calculate total strategy rho")
          .def("total_vanna", &optipricer::strategies::OptionsStrategy::total_vanna,
               "Calculate total strategy vanna")
          .def("total_volga", &optipricer::strategies::OptionsStrategy::total_volga,
               "Calculate total strategy volga")
          .def("total_charm", &optipricer::strategies::OptionsStrategy::total_charm,
               "Calculate total strategy charm")
          .def("payoff_at_expiration", &optipricer::strategies::OptionsStrategy::payoff_at_expiration,
               "Calculate payoff at expiration for given underlying price",
               py::arg("underlying_price"))
          .def("get_positions", &optipricer::strategies::OptionsStrategy::get_positions,
               "Get all positions in the strategy")
          .def("get_name", &optipricer::strategies::OptionsStrategy::get_name,
               "Get strategy name")
          .def("get_dividend_yield", &optipricer::strategies::OptionsStrategy::get_dividend_yield,
               "Get dividend yield")
          .def("__repr__", [](const optipricer::strategies::OptionsStrategy &s) {
               std::string pos_str = "[";
               const auto& positions = s.get_positions();
               for (size_t i = 0; i < positions.size(); ++i) {
                    std::string opt_type = (positions[i].option_type == optipricer::strategies::OptionType::CALL) ? "CALL" : "PUT";
                    std::string pos_type = (positions[i].position_type == optipricer::strategies::PositionType::LONG) ? "LONG" : "SHORT";
                    pos_str += "Position(" + opt_type + ", " + pos_type + ", qty=" + format_double(positions[i].quantity) + ", K=" + format_double(positions[i].strike) + ")";
                    if (i < positions.size() - 1) {
                         pos_str += ", ";
                    }
               }
               pos_str += "]";
               return "OptionsStrategy(name=\"" + s.get_name() + "\", positions=" + pos_str + ")";
          });

     // Specific strategy classes
     py::class_<optipricer::strategies::LongCall, optipricer::strategies::OptionsStrategy>(strategies, "LongCall")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::ShortCall, optipricer::strategies::OptionsStrategy>(strategies, "ShortCall")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::LongPut, optipricer::strategies::OptionsStrategy>(strategies, "LongPut")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::ShortPut, optipricer::strategies::OptionsStrategy>(strategies, "ShortPut")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::LongStraddle, optipricer::strategies::OptionsStrategy>(strategies, "LongStraddle")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::ShortStraddle, optipricer::strategies::OptionsStrategy>(strategies, "ShortStraddle")
          .def(py::init<double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::LongStrangle, optipricer::strategies::OptionsStrategy>(strategies, "LongStrangle")
          .def(py::init<double, double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);

     py::class_<optipricer::strategies::ShortStrangle, optipricer::strategies::OptionsStrategy>(strategies, "ShortStrangle")
          .def(py::init<double, double, double, double, double, double, double, double>(),
               py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
               py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0, py::arg("dividend_yield") = 0.0);
}
