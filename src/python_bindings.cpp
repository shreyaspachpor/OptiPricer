#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "optipricer/models.hpp"
#include "optipricer/greeks.hpp"
#include "optipricer/strategies.hpp"
#include "optipricer/utils.hpp"

namespace py = pybind11;

PYBIND11_MODULE(optipricer, m)
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
         .def(py::init<double, double, double, double, double>(),
              "Initialize Black-Scholes model\n\n"
              "Parameters:\n"
              "  strike_price: Option strike price (K) - must be positive\n"
              "  volatility: Annualized volatility (sigma) - must be non-negative\n"
              "  risk_free_rate: Risk-free interest rate (r) - annualized\n"
              "  time_to_maturity: Time to expiration (T) in years - must be positive\n"
              "  underlying_price: Current underlying asset price (S) - must be positive\n\n"
              "Raises:\n"
              "  ValueError: If any parameter is invalid (negative, zero, NaN, or infinite)",
              py::arg("strike_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("underlying_price"))
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
              "Get underlying price");

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
              "Calculate put option rho");

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
         .def_readwrite("strike", &optipricer::strategies::Position::strike);

     py::class_<optipricer::strategies::OptionsStrategy>(strategies, "OptionsStrategy")
         .def("add_position", &optipricer::strategies::OptionsStrategy::add_position,
              "Add a position to the strategy",
              py::arg("option_type"), py::arg("position_type"), py::arg("quantity"), py::arg("strike"))
         .def("total_value", &optipricer::strategies::OptionsStrategy::total_value,
              "Calculate total strategy value")
         .def("total_delta", &optipricer::strategies::OptionsStrategy::total_delta,
              "Calculate total strategy delta")
         .def("payoff_at_expiration", &optipricer::strategies::OptionsStrategy::payoff_at_expiration,
              "Calculate payoff at expiration for given underlying price",
              py::arg("underlying_price"))
         .def("get_positions", &optipricer::strategies::OptionsStrategy::get_positions,
              "Get all positions in the strategy")
         .def("get_name", &optipricer::strategies::OptionsStrategy::get_name,
              "Get strategy name");

     // Specific strategy classes
     py::class_<optipricer::strategies::LongCall, optipricer::strategies::OptionsStrategy>(strategies, "LongCall")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::ShortCall, optipricer::strategies::OptionsStrategy>(strategies, "ShortCall")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::LongPut, optipricer::strategies::OptionsStrategy>(strategies, "LongPut")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::ShortPut, optipricer::strategies::OptionsStrategy>(strategies, "ShortPut")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::LongStraddle, optipricer::strategies::OptionsStrategy>(strategies, "LongStraddle")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::ShortStraddle, optipricer::strategies::OptionsStrategy>(strategies, "ShortStraddle")
         .def(py::init<double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::LongStrangle, optipricer::strategies::OptionsStrategy>(strategies, "LongStrangle")
         .def(py::init<double, double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0);

     py::class_<optipricer::strategies::ShortStrangle, optipricer::strategies::OptionsStrategy>(strategies, "ShortStrangle")
         .def(py::init<double, double, double, double, double, double, double>(),
              py::arg("underlying_price"), py::arg("volatility"), py::arg("risk_free_rate"),
              py::arg("time_to_maturity"), py::arg("put_strike"), py::arg("call_strike"), py::arg("quantity") = 1.0);
}
