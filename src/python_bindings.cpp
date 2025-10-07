#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "optiverse/models.hpp"
#include "optiverse/greeks.hpp"
#include "optiverse/strategies.hpp"

namespace py = pybind11;

PYBIND11_MODULE(optiverse, m) {
    m.doc() = "OptiVerse: A comprehensive options pricing and analysis library";
    
    // Utility functions
    m.def("norm_cdf", &optiverse::norm_cdf, "Standard normal cumulative distribution function",
          py::arg("x"));
    m.def("norm_pdf", &optiverse::norm_pdf, "Standard normal probability density function",
          py::arg("x"));
    
    // Models submodule
    py::module_ models = m.def_submodule("models", "Options pricing models");
    
    py::class_<optiverse::models::BlackScholesModel>(models, "BlackScholesModel")
        .def(py::init<double, double, double, double, double>(),
             "Initialize Black-Scholes model",
             py::arg("strike_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("underlying_price"))
        .def("d1", &optiverse::models::BlackScholesModel::d1,
             "Calculate d1 parameter")
        .def("d2", &optiverse::models::BlackScholesModel::d2,
             "Calculate d2 parameter")
        .def("call_price", &optiverse::models::BlackScholesModel::call_price,
             "Calculate call option price")
        .def("put_price", &optiverse::models::BlackScholesModel::put_price,
             "Calculate put option price")
        .def("get_strike_price", &optiverse::models::BlackScholesModel::get_strike_price,
             "Get strike price")
        .def("get_volatility", &optiverse::models::BlackScholesModel::get_volatility,
             "Get volatility")
        .def("get_risk_free_rate", &optiverse::models::BlackScholesModel::get_risk_free_rate,
             "Get risk-free rate")
        .def("get_time_to_maturity", &optiverse::models::BlackScholesModel::get_time_to_maturity,
             "Get time to maturity")
        .def("get_underlying_price", &optiverse::models::BlackScholesModel::get_underlying_price,
             "Get underlying price");
    
    py::class_<optiverse::models::GreeksCalculator>(models, "GreeksCalculator")
        .def(py::init<const optiverse::models::BlackScholesModel&>(),
             "Initialize Greeks calculator with Black-Scholes model",
             py::arg("model"))
        .def("call_delta", &optiverse::models::GreeksCalculator::call_delta,
             "Calculate call option delta")
        .def("put_delta", &optiverse::models::GreeksCalculator::put_delta,
             "Calculate put option delta")
        .def("gamma", &optiverse::models::GreeksCalculator::gamma,
             "Calculate gamma")
        .def("vega", &optiverse::models::GreeksCalculator::vega,
             "Calculate vega")
        .def("call_theta", &optiverse::models::GreeksCalculator::call_theta,
             "Calculate call option theta")
        .def("put_theta", &optiverse::models::GreeksCalculator::put_theta,
             "Calculate put option theta")
        .def("call_rho", &optiverse::models::GreeksCalculator::call_rho,
             "Calculate call option rho")
        .def("put_rho", &optiverse::models::GreeksCalculator::put_rho,
             "Calculate put option rho");
    
    // Strategies submodule
    py::module_ strategies = m.def_submodule("strategies", "Options trading strategies");
    
    py::enum_<optiverse::strategies::OptionType>(strategies, "OptionType")
        .value("CALL", optiverse::strategies::OptionType::CALL)
        .value("PUT", optiverse::strategies::OptionType::PUT);
    
    py::enum_<optiverse::strategies::PositionType>(strategies, "PositionType")
        .value("LONG", optiverse::strategies::PositionType::LONG)
        .value("SHORT", optiverse::strategies::PositionType::SHORT);
    
    py::class_<optiverse::strategies::Position>(strategies, "Position")
        .def(py::init<optiverse::strategies::OptionType, optiverse::strategies::PositionType, double, double>(),
             py::arg("option_type"), py::arg("position_type"), py::arg("quantity"), py::arg("strike"))
        .def_readwrite("option_type", &optiverse::strategies::Position::option_type)
        .def_readwrite("position_type", &optiverse::strategies::Position::position_type)
        .def_readwrite("quantity", &optiverse::strategies::Position::quantity)
        .def_readwrite("strike", &optiverse::strategies::Position::strike);
    
    py::class_<optiverse::strategies::OptionsStrategy>(strategies, "OptionsStrategy")
        .def("add_position", &optiverse::strategies::OptionsStrategy::add_position,
             "Add a position to the strategy",
             py::arg("option_type"), py::arg("position_type"), py::arg("quantity"), py::arg("strike"))
        .def("total_value", &optiverse::strategies::OptionsStrategy::total_value,
             "Calculate total strategy value")
        .def("total_delta", &optiverse::strategies::OptionsStrategy::total_delta,
             "Calculate total strategy delta")
        .def("payoff_at_expiration", &optiverse::strategies::OptionsStrategy::payoff_at_expiration,
             "Calculate payoff at expiration for given underlying price",
             py::arg("underlying_price"))
        .def("get_positions", &optiverse::strategies::OptionsStrategy::get_positions,
             "Get all positions in the strategy")
        .def("get_name", &optiverse::strategies::OptionsStrategy::get_name,
             "Get strategy name");
    
    // Specific strategy classes
    py::class_<optiverse::strategies::LongCall, optiverse::strategies::OptionsStrategy>(strategies, "LongCall")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::ShortCall, optiverse::strategies::OptionsStrategy>(strategies, "ShortCall")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::LongPut, optiverse::strategies::OptionsStrategy>(strategies, "LongPut")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::ShortPut, optiverse::strategies::OptionsStrategy>(strategies, "ShortPut")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::LongStraddle, optiverse::strategies::OptionsStrategy>(strategies, "LongStraddle")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::ShortStraddle, optiverse::strategies::OptionsStrategy>(strategies, "ShortStraddle")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::LongStrangle, optiverse::strategies::OptionsStrategy>(strategies, "LongStrangle")
        .def(py::init<double, double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0);
    
    py::class_<optiverse::strategies::ShortStrangle, optiverse::strategies::OptionsStrategy>(strategies, "ShortStrangle")
        .def(py::init<double, double, double, double, double, double, double>(),
             py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
             py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0);
}
