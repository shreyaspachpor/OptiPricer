# OpticPricer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/downloads/)

A high-performance Python library for options pricing, Greeks calculation, and derivatives strategies analysis. Built with C++ for speed and wrapped with Python for ease of use.

## Features

- Black-Scholes-Merton Model for European options pricing
- Greeks calculation (Delta, Gamma, Theta, Vega, Rho)
- Options strategies (Calls, Puts, Straddles, Strangles)
- High performance C++ implementation with Python bindings

## Installation

### Prerequisites
- Python 3.7 or higher
- C++ compiler (automatically handled by pip)

### Install from PyPI
```bash
pip install optipricer
```

### Verify Installation
```python
import optipricer
print("Optipricer installed successfully!")
```

## Quick Start

### Basic Options Pricing
```python
import optipricer

# Create a Black-Scholes model
model = optipricer.models.BlackScholesModel(
    strike_price=105.0,      # Strike price (K)
    volatility=0.25,         # 25% volatility (σ)
    risk_free_rate=0.05,     # 5% risk-free rate (r)
    time_to_maturity=0.25,   # 3 months to expiration (T)
    underlying_price=100.0   # Current stock price (S)
)

# Calculate option prices
call_price = model.call_price()
put_price = model.put_price()

print(f"Call Price: ${call_price:.2f}")
print(f"Put Price: ${put_price:.2f}")

# Expected output:
# Call Price: $3.44
# Put Price: $7.14
```

### Greeks Analysis
```python
# Create Greeks calculator
greeks = optipricer.models.GreeksCalculator(model)

# Calculate Greeks
delta_call = greeks.call_delta()
gamma = greeks.gamma()
vega = greeks.vega()
theta_call = greeks.call_theta()

print(f"Call Delta: {delta_call:.4f}")
print(f"Gamma: {gamma:.4f}")
print(f"Vega: {vega:.2f}")
print(f"Theta: {theta_call:.4f}")

# Expected output:
# Call Delta: 0.4099
# Gamma: 0.0311
# Vega: 0.19
# Theta: -0.0318
```

### Options Strategies
```python
# Long Straddle Strategy
straddle = optipricer.strategies.LongStraddle(
    underlying_price=100.0,
    volatility=0.25,
    risk_free_rate=0.05,
    time_to_maturity=0.25,
    strike=100.0,
    quantity=1.0
)

# Analyze strategy
current_value = straddle.total_value()
total_delta = straddle.total_delta()

print(f"Strategy Value: ₹{current_value:.2f}")
print(f"Portfolio Delta: {total_delta:.4f}")

# Expected output:
# Strategy Value: ₹9.95
# Portfolio Delta: 0.1291
```

## API Reference

### BlackScholesModel
```python
BlackScholesModel(strike_price, volatility, risk_free_rate, time_to_maturity, underlying_price)
```

**Parameters:**
- `strike_price` (float): Option strike price
- `volatility` (float): Underlying asset volatility (annualized)
- `risk_free_rate` (float): Risk-free interest rate (annualized)
- `time_to_maturity` (float): Time to expiration in years
- `underlying_price` (float): Current price of underlying asset

**Methods:**
- `call_price()` → Calculate call option price
- `put_price()` → Calculate put option price
- `d1()` → Calculate d1 parameter
- `d2()` → Calculate d2 parameter

### GreeksCalculator
```python
GreeksCalculator(model: BlackScholesModel)
```

**Methods:**
- `call_delta()` / `put_delta()` → Price sensitivity to underlying
- `gamma()` → Delta sensitivity to underlying
- `vega()` → Price sensitivity to volatility
- `call_theta()` / `put_theta()` → Price sensitivity to time
- `call_rho()` / `put_rho()` → Price sensitivity to interest rate

### Strategy Classes
Available strategies:
- `LongCall` / `ShortCall`
- `LongPut` / `ShortPut`
- `LongStraddle` / `ShortStraddle`
- `LongStrangle` / `ShortStrangle`

**Common Methods:**
- `total_value()` → Current strategy value
- `total_delta()` → Portfolio delta
- `payoff_at_expiration(price)` → Payoff at expiration

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
