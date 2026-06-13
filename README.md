# OptiPricer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/downloads/)
[![CI/CD](https://github.com/shreyaspachpor/OptiPricer/actions/workflows/ci.yml/badge.svg)](https://github.com/shreyaspachpor/OptiPricer/actions)

A high-performance options pricing, Greeks sensitivity, and strategy analysis library. OptiPricer combines the sheer speed of a C++ core with a clean, pythonic wrapper to deliver quick calculations and interactive visualizations. It is optimized for European-style stock and index options (such as those traded on the National Stock Exchange of India, NSE).

---

## Features

- **High-Performance C++ Core**: Heavy math and iterative solvers are compiled to native code, wrapped via `pybind11` for maximal performance.
- **Pythonic Facade API**: Simple, clean one-liners for pricing, Greeks, and implied volatilities.
- **Continuous Dividend Yield Support** ($q$): Integrated across all model layers (crucial for pricing index options like NIFTY).
- **Implied Volatility (IV) Solver**: Fast, robust hybrid Newton-Raphson & Bisection root finder.
- **First & Second-Order Greeks**: Full suite including Delta, Gamma, Vega, Theta, Rho, **Vanna**, **Volga**, and **Charm**.
- **Advanced Options Strategies**: Model complex portfolios like Straddles, Strangles, Bull/Bear Spreads, and Iron Condors with full portfolio-level Greeks.
- **Option Chain Builder**: Generate broker-terminal-style option chains with prices, Greeks, and IVs across strikes.
- **Volatility Surface**: Build, interpolate, and visualize implied volatility surfaces across strikes and expiries.
- **Interactive Visualizations**: Payoff profiles, Greek sensitivity curves, volatility smiles, and 3D vol surface plots.

---

## Installation

### Prerequisites
- Python 3.7 or higher
- A C++ compiler (automatically configured by pip during build)

### Standard Install
```bash
pip install optipricer
```

### Install with Optional Dependencies
```bash
pip install optipricer[viz]    # Matplotlib for plotting
pip install optipricer[data]   # Pandas for DataFrames
pip install optipricer[all]    # Everything
```

---

## Quick Start

### 1. High-Level Facade API (One-Liners)

For rapid pricing and basic tasks, you can calculate option values directly:

```python
import optipricer

# Calculate Black-Scholes-Merton option price (with 3% continuous dividend yield)
call_price = optipricer.price(S=100.0, K=105.0, r=0.05, T=0.25, vol=0.25, q=0.03, option='call')
print(f"Call Option Price: INR {call_price:.2f}")

# Calculate all Greeks at once (returns a dictionary)
g = optipricer.greeks(S=100.0, K=105.0, r=0.05, T=0.25, vol=0.25, q=0.03)
print(f"Call Delta: {g['call_delta']:.4f} | Gamma: {g['gamma']:.4f} | Vega: {g['vega']:.4f}")
print(f"Vanna: {g['vanna']:.4f} | Volga: {g['volga']:.4f} | Charm: {g['call_charm']:.6f}")

# Calculate Implied Volatility given a market premium
iv = optipricer.implied_vol(market_price=3.20, S=100.0, K=105.0, r=0.05, T=0.25, q=0.03, option='call')
print(f"Solved Implied Volatility: {iv * 100:.2f}%")
```

---

### 2. Advanced Object-Oriented C++ API

For caching inputs, reusing setups, and performance-critical loops:

```python
from optipricer.models import BlackScholesModel, GreeksCalculator

# Instantiate the C++ model object (saves input verification overhead in tight loops)
model = BlackScholesModel(
    strike_price=105.0,
    volatility=0.25,
    risk_free_rate=0.05,
    time_to_maturity=0.25,
    underlying_price=100.0,
    dividend_yield=0.03
)

print(model)
# Output: BlackScholesModel(strike_price=105, volatility=0.25, risk_free_rate=0.05, time_to_maturity=0.25, underlying_price=100, dividend_yield=0.03)

# Calculate prices
print(f"Call Price: INR {model.call_price():.2f}")

# Calculate Greeks (including second-order)
greeks = GreeksCalculator(model)
print(f"Call Theta (per day decay): {greeks.call_theta():.4f}")
print(f"Vanna (dDelta/dVol): {greeks.vanna():.4f}")
print(f"Volga (dVega/dVol): {greeks.volga():.4f}")
print(f"Charm (delta decay/day): {greeks.call_charm():.6f}")
```

---

### 3. Option Trading Strategies

Model multi-leg options portfolios. Compute strategy net premiums, **complete portfolio-level Greeks**, and expiration payoffs:

```python
from optipricer.strategies import IronCondor

# Set up an Iron Condor strategy for NIFTY Index Options
ic = IronCondor(
    S=21500.0,
    sigma=0.16,      # Implied Volatility
    r=0.07,          # 7% Risk-Free Rate
    T=30 / 365.0,    # 30 days to expiration
    put_strike_long=21100,
    put_strike_short=21300,
    call_strike_short=21700,
    call_strike_long=21900,
    q=0.012          # NIFTY Dividend Yield
)

# Strategy value (negative represents net credit received)
print(f"Strategy Premium Value: INR {ic.total_value():.2f}")

# Full portfolio-level Greeks
print(f"Portfolio Delta: {ic.total_delta():.4f}")
print(f"Portfolio Gamma: {ic.total_gamma():.6f}")
print(f"Portfolio Vega:  {ic.total_vega():.4f}")
print(f"Portfolio Theta: {ic.total_theta():.4f}")
print(f"Portfolio Rho:   {ic.total_rho():.4f}")
```

---

### 4. Option Chain Builder

Generate a broker-terminal-style option chain across multiple strikes:

```python
from optipricer.chain import OptionChain, generate_nifty_strikes

# Generate NIFTY-style strikes centered around spot
strikes = generate_nifty_strikes(spot=21500.0, step=100.0, count=11)

# Build the chain
chain = OptionChain(S=21500.0, r=0.07, T=30/365.0, strikes=strikes, vol=0.16, q=0.012)
data = chain.build()

# Print a summary
for row in data:
    print(f"K={row['strike']:>8.0f}  Call={row['call_price']:>8.2f}  Put={row['put_price']:>8.2f}  "
          f"Δc={row['call_delta']:>+.4f}  Δp={row['put_delta']:>+.4f}  Γ={row['gamma']:.6f}")

# Convert to pandas DataFrame (requires pip install pandas)
# df = chain.to_dataframe()
```

---

### 5. Volatility Surface

Build and interpolate implied volatility across strikes and expiries:

```python
from optipricer.surface import VolatilitySurface

# Construct from market data (e.g., two expiry slices)
chain_data = [
    {'expiry': 7/365,  'strikes': [21200, 21300, 21400, 21500, 21600, 21700, 21800],
     'ivs': [0.18, 0.17, 0.16, 0.155, 0.16, 0.17, 0.18]},
    {'expiry': 30/365, 'strikes': [21200, 21300, 21400, 21500, 21600, 21700, 21800],
     'ivs': [0.17, 0.165, 0.155, 0.15, 0.155, 0.165, 0.17]},
]
surface = VolatilitySurface.from_chain_data(chain_data)

# Interpolate IV at any (strike, expiry) point
iv = surface.get_iv(strike=21450, expiry=15/365)
print(f"Interpolated IV: {iv * 100:.2f}%")

# Extract a volatility smile for the nearest expiry
strikes, ivs = surface.smile(expiry_index=0)
```

---

## Visualizing Payoffs & Greek Sensitivities

Generate professional charts of option payoffs, Greek curves, and volatility surfaces:

```python
import optipricer.viz
import matplotlib.pyplot as plt

# 1. Plot strategy expiration payoff profile (gross payoff and net P&L zones)
optipricer.viz.plot_payoff(ic)
plt.savefig("nifty_iron_condor_payoff.png")
plt.close()

# 2. Plot Greek sensitivity (e.g. Call Delta vs. Underlying Asset Price)
optipricer.viz.plot_greek_sensitivity(
    greek_name='delta',
    variable_name='underlying_price',
    variable_range=range(80, 130),
    S=100.0, K=105.0, r=0.05, T=0.25, vol=0.25, q=0.03,
    option_type='call'
)
plt.savefig("call_delta_vs_spot.png")
plt.close()

# 3. Plot 3D volatility surface
surface.plot()
plt.savefig("vol_surface.png")
plt.close()

# 4. Plot 2D volatility smile
surface.plot_smile(expiry_index=0)
plt.savefig("vol_smile.png")
plt.close()
```

---

## Greeks Reference

OptiPricer computes both first-order and second-order sensitivity measures:

| Greek | Symbol | Description | Available On |
| :--- | :--- | :--- | :--- |
| **Delta** | Δ | Price sensitivity to underlying | Call/Put, Strategy |
| **Gamma** | Γ | Delta sensitivity to underlying | Shared, Strategy |
| **Vega** | ν | Price sensitivity to volatility | Shared, Strategy |
| **Theta** | Θ | Time decay (per day) | Call/Put, Strategy |
| **Rho** | ρ | Price sensitivity to interest rate | Call/Put, Strategy |
| **Vanna** | ∂Δ/∂σ | Delta sensitivity to volatility | Shared, Strategy |
| **Volga** | ∂²V/∂σ² | Vega convexity (Vomma) | Shared, Strategy |
| **Charm** | ∂Δ/∂t | Delta decay over time (per day) | Call/Put, Strategy |

---

## Performance Benchmarks

Run the benchmark suite on your machine:

```bash
python benchmarks/benchmark.py
```

Typical results on a modern CPU:

| Operation | Pure Python | OptiPricer (C++) | Speedup |
| :--- | :--- | :--- | :--- |
| **100,000 Option Price Evaluations** | ~0.10s | ~0.05s | **~2x** |
| **10,000 Implied Volatility Solves** | ~0.10s | ~0.03s | **~3.7x** |

### Key takeaways:
1. For simple calculations, caching a C++ `BlackScholesModel` object is **~2x faster** than Python. 
2. For iterative numerical solvers (like implied volatility root-finding), OptiPricer achieves **~3.7x speedup** because the entire loop executes within optimized, native machine instructions.

---

## Project Structure

```
OptiPricer/
├── include/optipricer/      # C++ header-only library
│   ├── models.hpp            # Black-Scholes model + IV solver
│   ├── greeks.hpp            # First & second-order Greeks calculator
│   ├── strategies.hpp        # Strategy composition engine
│   └── utils.hpp             # Math utilities (norm_cdf, norm_pdf)
├── src/
│   └── python_bindings.cpp   # Pybind11 bindings
├── optipricer/               # Python package
│   ├── __init__.py           # Facade API (price, greeks, implied_vol)
│   ├── models.py             # C++ model wrappers
│   ├── strategies.py         # Python-extended strategies (Spreads, Condors)
│   ├── chain.py              # Option chain builder
│   ├── surface.py            # Volatility surface interpolation
│   ├── viz.py                # Visualization helpers
│   ├── nse.py                # NSE-specific utilities
│   └── _core.pyi             # Type stubs for IDE autocompletion
├── tests/python/             # Pytest test suite
├── benchmarks/               # Performance benchmarks
├── examples/                 # Usage examples and notebooks
└── .github/workflows/        # CI/CD pipeline
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
