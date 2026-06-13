"""
OptiPricer: A high-performance options pricing and analysis library.
Optimized for European-style stock and index options.
"""

from . import models
from . import strategies
from . import nse

# Lazy import for viz — only loaded when explicitly accessed.
# This avoids polluting the namespace when matplotlib is not installed.
def __getattr__(name):
    if name == "viz":
        import importlib
        _viz = importlib.import_module(".viz", __name__)
        globals()["viz"] = _viz
        return _viz
    raise AttributeError(f"module 'optipricer' has no attribute {name!r}")

# High-level Facade API
def price(S: float, K: float, r: float, T: float, vol: float, q: float = 0.0, option: str = 'call') -> float:
    """
    Calculate the Black-Scholes-Merton option price.

    Parameters:
        S (float): Current price of the underlying asset
        K (float): Strike price of the option
        r (float): Annualized risk-free interest rate (e.g. 0.05 for 5%)
        T (float): Time to maturity in years (e.g. 0.25 for 3 months)
        vol (float): Annualized volatility of the underlying asset (e.g. 0.20 for 20%)
        q (float): Annualized continuous dividend yield (default 0.0)
        option (str): Option type, either 'call' or 'put' (default 'call')

    Returns:
        float: Calculated option price
    """
    opt = option.lower().strip()
    if opt not in ('call', 'put'):
        raise ValueError(f"Invalid option type: '{option}'. Must be 'call' or 'put'.")
    
    model = models.BlackScholesModel(strike_price=K, volatility=vol, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q)
    return model.call_price() if opt == 'call' else model.put_price()

def greeks(S: float, K: float, r: float, T: float, vol: float, q: float = 0.0) -> dict:
    """
    Calculate option sensitivity measures (Greeks) for both call and put options.
    Includes first-order Greeks (Delta, Gamma, Vega, Theta, Rho) and
    second-order Greeks (Vanna, Volga, Charm).

    Parameters:
        S (float): Current price of the underlying asset
        K (float): Strike price of the option
        r (float): Annualized risk-free interest rate
        T (float): Time to maturity in years
        vol (float): Annualized volatility
        q (float): Annualized continuous dividend yield (default 0.0)

    Returns:
        dict: A dictionary containing all computed Greeks:
              First-order: 'call_delta', 'put_delta', 'gamma', 'vega', 'call_theta', 'put_theta', 'call_rho', 'put_rho'
              Second-order: 'vanna', 'volga', 'call_charm', 'put_charm'
    """
    model = models.BlackScholesModel(strike_price=K, volatility=vol, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q)
    calc = models.GreeksCalculator(model)
    return {
        # First-order Greeks
        'call_delta': calc.call_delta(),
        'put_delta': calc.put_delta(),
        'gamma': calc.gamma(),
        'vega': calc.vega(),
        'call_theta': calc.call_theta(),
        'put_theta': calc.put_theta(),
        'call_rho': calc.call_rho(),
        'put_rho': calc.put_rho(),
        # Second-order Greeks
        'vanna': calc.vanna(),
        'volga': calc.volga(),
        'call_charm': calc.call_charm(),
        'put_charm': calc.put_charm(),
    }

def implied_vol(market_price: float, S: float, K: float, r: float, T: float, q: float = 0.0, option: str = 'call', tol: float = 1e-6, max_iter: int = 100) -> float:
    """
    Calculate the implied volatility of an option given its market price.

    Parameters:
        market_price (float): Market price of the option
        S (float): Current price of the underlying asset
        K (float): Strike price of the option
        r (float): Annualized risk-free interest rate
        T (float): Time to maturity in years
        q (float): Annualized continuous dividend yield (default 0.0)
        option (str): Option type, either 'call' or 'put' (default 'call')
        tol (float): Convergence tolerance for solver (default 1e-6)
        max_iter (int): Maximum number of iterations for solver (default 100)

    Returns:
        float: Solved implied volatility (e.g. 0.25 for 25%)
    """
    opt = option.lower().strip()
    if opt not in ('call', 'put'):
        raise ValueError(f"Invalid option type: '{option}'. Must be 'call' or 'put'.")
    
    is_call = (opt == 'call')
    return models.calculate_implied_volatility(market_price, strike_price=K, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q, is_call=is_call, tol=tol, max_iter=max_iter)

__all__ = ['price', 'greeks', 'implied_vol', 'models', 'strategies', 'nse', 'viz']
