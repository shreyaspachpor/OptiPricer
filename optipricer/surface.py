"""
Volatility Surface module for OptiPricer.

Models implied volatility across strikes and expiries, enabling
interpolation and 3D visualization of the vol surface/smile.
"""

import math


class VolatilitySurface:
    """
    Stores and interpolates implied volatility data across a grid of
    strikes and expiries (time to maturity).

    The surface can be constructed from raw IV data (e.g., from NSE option
    chain snapshots) and provides bilinear interpolation for arbitrary
    (strike, expiry) queries.

    Parameters:
        strikes (list[float]): Sorted list of strike prices
        expiries (list[float]): Sorted list of expiry times (in years)
        iv_matrix (list[list[float]]): 2D matrix of IVs, where
            iv_matrix[i][j] is the IV for expiries[i] and strikes[j].
            Shape: (len(expiries), len(strikes))
    """

    def __init__(self, strikes: list, expiries: list, iv_matrix: list):
        if not strikes or not expiries:
            raise ValueError("strikes and expiries must be non-empty.")
        if len(iv_matrix) != len(expiries):
            raise ValueError(
                f"iv_matrix has {len(iv_matrix)} rows but expected {len(expiries)} (one per expiry)."
            )
        for i, row in enumerate(iv_matrix):
            if len(row) != len(strikes):
                raise ValueError(
                    f"iv_matrix row {i} has {len(row)} columns but expected {len(strikes)} (one per strike)."
                )

        self.strikes = sorted(strikes)
        self.expiries = sorted(expiries)
        # Re-sort matrix rows to match sorted expiries
        self.iv_matrix = iv_matrix

    @classmethod
    def from_chain_data(cls, chain_data: list) -> 'VolatilitySurface':
        """
        Construct a VolatilitySurface from a list of chain snapshots.

        Parameters:
            chain_data (list[dict]): Each dict must have:
                - 'expiry' (float): Time to maturity in years
                - 'strikes' (list[float]): Strike prices
                - 'ivs' (list[float]): Implied volatilities matching strikes

        Returns:
            VolatilitySurface: Constructed surface object
        """
        if not chain_data:
            raise ValueError("chain_data must not be empty.")

        # Collect all unique strikes across all expiries
        all_strikes = set()
        for entry in chain_data:
            all_strikes.update(entry['strikes'])
        strikes = sorted(all_strikes)

        # Sort by expiry
        chain_data_sorted = sorted(chain_data, key=lambda x: x['expiry'])
        expiries = [entry['expiry'] for entry in chain_data_sorted]

        # Build the IV matrix, using NaN for missing data
        iv_matrix = []
        for entry in chain_data_sorted:
            iv_dict = dict(zip(entry['strikes'], entry['ivs']))
            row = [iv_dict.get(k, float('nan')) for k in strikes]
            iv_matrix.append(row)

        return cls(strikes, expiries, iv_matrix)

    def get_iv(self, strike: float, expiry: float) -> float:
        """
        Get interpolated implied volatility for a given strike and expiry.
        Uses bilinear interpolation on the IV grid.

        Parameters:
            strike (float): Strike price to query
            expiry (float): Time to expiry in years

        Returns:
            float: Interpolated implied volatility
        """
        # Find bounding indices for strike
        si = self._find_bracket(self.strikes, strike)
        # Find bounding indices for expiry
        ei = self._find_bracket(self.expiries, expiry)

        # Bilinear interpolation
        s0, s1 = self.strikes[si], self.strikes[min(si + 1, len(self.strikes) - 1)]
        e0, e1 = self.expiries[ei], self.expiries[min(ei + 1, len(self.expiries) - 1)]

        # Get the 4 corner IVs
        iv00 = self.iv_matrix[ei][si]
        iv01 = self.iv_matrix[ei][min(si + 1, len(self.strikes) - 1)]
        iv10 = self.iv_matrix[min(ei + 1, len(self.expiries) - 1)][si]
        iv11 = self.iv_matrix[min(ei + 1, len(self.expiries) - 1)][min(si + 1, len(self.strikes) - 1)]

        # Handle edge cases (exact match or boundary)
        if s0 == s1:
            t_s = 0.0
        else:
            t_s = (strike - s0) / (s1 - s0)

        if e0 == e1:
            t_e = 0.0
        else:
            t_e = (expiry - e0) / (e1 - e0)

        # Clamp to [0, 1]
        t_s = max(0.0, min(1.0, t_s))
        t_e = max(0.0, min(1.0, t_e))

        # Bilinear
        iv_top = iv00 * (1 - t_s) + iv01 * t_s
        iv_bot = iv10 * (1 - t_s) + iv11 * t_s
        return iv_top * (1 - t_e) + iv_bot * t_e

    def smile(self, expiry_index: int = 0) -> tuple:
        """
        Extract a volatility smile (IV vs strike) for a specific expiry.

        Parameters:
            expiry_index (int): Index into the expiries list (default 0 = nearest expiry)

        Returns:
            tuple: (strikes, ivs) — two lists of equal length
        """
        if expiry_index < 0 or expiry_index >= len(self.expiries):
            raise IndexError(f"expiry_index {expiry_index} out of range [0, {len(self.expiries) - 1}]")
        return list(self.strikes), list(self.iv_matrix[expiry_index])

    def term_structure(self, strike: float) -> tuple:
        """
        Extract the term structure (IV vs expiry) for a specific strike.

        Parameters:
            strike (float): The strike price to query

        Returns:
            tuple: (expiries, ivs) — two lists of equal length
        """
        si = self._find_nearest(self.strikes, strike)
        ivs = [self.iv_matrix[ei][si] for ei in range(len(self.expiries))]
        return list(self.expiries), ivs

    def plot(self, title: str = None):
        """
        Plot the volatility surface as a 3D mesh.

        Returns:
            matplotlib.figure.Figure: The generated figure
        """
        try:
            import matplotlib.pyplot as plt
            from mpl_toolkits.mplot3d import Axes3D
            import numpy as np
        except ImportError:
            raise ImportError(
                "matplotlib and numpy are required for plotting.\n"
                "Install with: pip install matplotlib numpy"
            )

        strikes_arr = np.array(self.strikes)
        expiries_arr = np.array(self.expiries)
        S, E = np.meshgrid(strikes_arr, expiries_arr)
        IV = np.array(self.iv_matrix) * 100  # Convert to percentage

        fig = plt.figure(figsize=(12, 8))
        ax = fig.add_subplot(111, projection='3d')
        surf = ax.plot_surface(S, E * 365, IV, cmap='viridis', alpha=0.85,
                               edgecolor='none', antialiased=True)

        ax.set_xlabel('Strike Price (K)', fontsize=11, labelpad=12)
        ax.set_ylabel('Days to Expiry', fontsize=11, labelpad=12)
        ax.set_zlabel('Implied Volatility (%)', fontsize=11, labelpad=12)
        ax.set_title(title or 'Implied Volatility Surface', fontsize=14, fontweight='bold', pad=20)

        fig.colorbar(surf, ax=ax, shrink=0.5, aspect=10, label='IV (%)')
        plt.tight_layout()
        return fig

    def plot_smile(self, expiry_index: int = 0, title: str = None):
        """
        Plot a 2D volatility smile for a specific expiry.

        Returns:
            matplotlib.figure.Figure: The generated figure
        """
        try:
            import matplotlib.pyplot as plt
            import numpy as np
        except ImportError:
            raise ImportError(
                "matplotlib and numpy are required for plotting.\n"
                "Install with: pip install matplotlib numpy"
            )

        strikes, ivs = self.smile(expiry_index)
        days = self.expiries[expiry_index] * 365

        plt.figure(figsize=(10, 6))
        plt.plot(strikes, [iv * 100 for iv in ivs], 'o-', color='#9b59b6',
                 linewidth=2.5, markersize=6, label=f'{days:.0f}-day smile')
        plt.title(title or f'Volatility Smile ({days:.0f} DTE)', fontsize=14, fontweight='bold', pad=15)
        plt.xlabel('Strike Price (K)', fontsize=11, labelpad=10)
        plt.ylabel('Implied Volatility (%)', fontsize=11, labelpad=10)
        plt.grid(True, linestyle=':', alpha=0.6)
        plt.legend(frameon=True, facecolor='white', edgecolor='none')
        plt.tight_layout()
        return plt.gcf()

    @staticmethod
    def _find_bracket(sorted_list: list, value: float) -> int:
        """Find the index i such that sorted_list[i] <= value < sorted_list[i+1]."""
        if value <= sorted_list[0]:
            return 0
        if value >= sorted_list[-1]:
            return len(sorted_list) - 1
        for i in range(len(sorted_list) - 1):
            if sorted_list[i] <= value < sorted_list[i + 1]:
                return i
        return len(sorted_list) - 2

    @staticmethod
    def _find_nearest(sorted_list: list, value: float) -> int:
        """Find the index of the nearest element."""
        return min(range(len(sorted_list)), key=lambda i: abs(sorted_list[i] - value))

    def __repr__(self) -> str:
        return (
            f"VolatilitySurface(strikes=[{self.strikes[0]}...{self.strikes[-1]}], "
            f"expiries=[{self.expiries[0]:.4f}...{self.expiries[-1]:.4f}], "
            f"shape=({len(self.expiries)}, {len(self.strikes)}))"
        )


__all__ = ['VolatilitySurface']
