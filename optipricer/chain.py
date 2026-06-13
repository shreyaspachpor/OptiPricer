"""
Option Chain builder for OptiPricer.

Generates a complete options chain (prices, Greeks, IVs) for a set of strikes,
similar to what a broker terminal displays.
"""

from .models import BlackScholesModel, GreeksCalculator


class OptionChain:
    """
    Generates a full option chain for a given underlying, similar to
    what is displayed on a broker terminal or NSE website.

    Each row contains call/put prices, all first and second-order Greeks,
    and can optionally solve implied volatilities from market data.

    Parameters:
        S (float): Current underlying price
        r (float): Risk-free rate (annualized)
        T (float): Time to expiry in years
        q (float): Continuous dividend yield (default 0.0)
        strikes (list[float]): List of strike prices to include
        vol (float, optional): Flat volatility to use for all strikes (if not using per-strike IVs)
        iv_map (dict, optional): Mapping of strike -> implied volatility (overrides flat vol)
    """

    def __init__(self, S: float, r: float, T: float, strikes: list,
                 vol: float = 0.20, q: float = 0.0, iv_map: dict = None):
        if not strikes:
            raise ValueError("strikes list must not be empty.")
        
        self.S = S
        self.r = r
        self.T = T
        self.q = q
        self.strikes = sorted(strikes)
        self.vol = vol
        self.iv_map = iv_map or {}
        self._chain = None  # Lazily computed

    def _get_vol(self, strike: float) -> float:
        """Get volatility for a specific strike (from iv_map or flat vol)."""
        return self.iv_map.get(strike, self.vol)

    def _build_row(self, strike: float) -> dict:
        """Build a single chain row for one strike."""
        sigma = self._get_vol(strike)
        model = BlackScholesModel(
            strike_price=strike, volatility=sigma, risk_free_rate=self.r,
            time_to_maturity=self.T, underlying_price=self.S, dividend_yield=self.q
        )
        calc = GreeksCalculator(model)

        return {
            'strike': strike,
            'iv': sigma,
            'call_price': model.call_price(),
            'put_price': model.put_price(),
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

    def build(self) -> list:
        """
        Build the full option chain.

        Returns:
            list[dict]: A list of dictionaries, one per strike, each containing
                        prices and Greeks for both call and put.
        """
        self._chain = [self._build_row(k) for k in self.strikes]
        return self._chain

    def to_dict(self) -> list:
        """
        Return the chain as a list of dicts (no external dependencies).

        Returns:
            list[dict]: The full option chain data.
        """
        if self._chain is None:
            self.build()
        return self._chain

    def to_dataframe(self):
        """
        Return the chain as a pandas DataFrame.

        Returns:
            pandas.DataFrame: Option chain with strikes as rows and metrics as columns.

        Raises:
            ImportError: If pandas is not installed.
        """
        try:
            import pandas as pd
        except ImportError:
            raise ImportError(
                "pandas is required for to_dataframe().\n"
                "Install it with: pip install pandas"
            )

        if self._chain is None:
            self.build()
        return pd.DataFrame(self._chain).set_index('strike')

    def atm_strike(self) -> float:
        """
        Find the at-the-money (ATM) strike — the one closest to current spot.

        Returns:
            float: The ATM strike price.
        """
        return min(self.strikes, key=lambda k: abs(k - self.S))

    def summary(self) -> dict:
        """
        Return a summary of the chain: ATM strike, total OI-weighted Greeks, etc.

        Returns:
            dict: Summary statistics of the chain.
        """
        if self._chain is None:
            self.build()

        atm = self.atm_strike()
        atm_row = next(r for r in self._chain if r['strike'] == atm)

        return {
            'underlying': self.S,
            'atm_strike': atm,
            'atm_call_price': atm_row['call_price'],
            'atm_put_price': atm_row['put_price'],
            'atm_iv': atm_row['iv'],
            'atm_call_delta': atm_row['call_delta'],
            'atm_gamma': atm_row['gamma'],
            'num_strikes': len(self.strikes),
            'min_strike': self.strikes[0],
            'max_strike': self.strikes[-1],
        }

    def __repr__(self) -> str:
        return (
            f"OptionChain(S={self.S}, T={self.T:.4f}, "
            f"strikes=[{self.strikes[0]}...{self.strikes[-1]}], "
            f"n={len(self.strikes)})"
        )


def generate_nifty_strikes(spot: float, step: float = 50.0, count: int = 21) -> list:
    """
    Generate NIFTY-style strike prices centered around the spot price.

    Parameters:
        spot (float): Current NIFTY spot price
        step (float): Strike interval (default 50 for NIFTY)
        count (int): Total number of strikes (default 21 for ±10 strikes)

    Returns:
        list[float]: Sorted list of strike prices
    """
    atm = round(spot / step) * step
    half = count // 2
    return [atm + (i - half) * step for i in range(count)]


__all__ = ['OptionChain', 'generate_nifty_strikes']
