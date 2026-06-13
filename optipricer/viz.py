def plot_payoff(strategy, underlying_range=None, title=None):
    """
    Plots the payoff and net profit/loss profile of an options strategy at expiration.
    Uses matplotlib to create a beautiful, high-quality chart.

    Parameters:
        strategy: An instance of optipricer.strategies.OptionsStrategy
        underlying_range (iterable, optional): Range of underlying prices to evaluate
        title (str, optional): Custom title for the plot
    """
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        raise ImportError(
            "The 'matplotlib' and 'numpy' libraries are required for plotting.\n"
            "Install them using: pip install matplotlib numpy\n"
            "Or install with viz dependencies: pip install optipricer[viz]"
        )

    positions = strategy.get_positions()
    if not positions:
        raise ValueError("Strategy has no positions to plot.")

    strikes = [pos.strike for pos in positions]
    min_strike = min(strikes)
    max_strike = max(strikes)

    # Automatically set a reasonable range if not provided
    if underlying_range is None:
        s_min = max(0.0, min_strike * 0.7)
        s_max = max_strike * 1.3
        underlying_range = np.linspace(s_min, s_max, 250)
    else:
        underlying_range = np.array(underlying_range)

    # Compute payoffs at expiration
    payoffs = np.array([strategy.payoff_at_expiration(s) for s in underlying_range])
    
    # Net profit includes premium entry cost (total strategy value)
    entry_cost = strategy.total_value()
    net_profits = payoffs - entry_cost

    plt.figure(figsize=(10, 6))
    
    # Plot curves
    plt.plot(underlying_range, payoffs, '--', color='#7f7f7f', label='Payoff at Expiry (Gross)', alpha=0.8, linewidth=1.5)
    plt.plot(underlying_range, net_profits, color='#1f77b4', label='Net Profit / Loss', linewidth=2.5)
    
    # Baseline
    plt.axhline(0, color='#2c3e50', linestyle='-', linewidth=1.2, alpha=0.7)
    
    # Shading profit/loss zones
    plt.fill_between(underlying_range, net_profits, 0, where=(net_profits > 0), facecolor='#2ecc71', alpha=0.2, label='Profit Zone')
    plt.fill_between(underlying_range, net_profits, 0, where=(net_profits < 0), facecolor='#e74c3c', alpha=0.2, label='Loss Zone')

    # Accentuate key strikes
    for strike in set(strikes):
        plt.axvline(strike, color='#95a5a6', linestyle=':', alpha=0.5, linewidth=1)
        plt.text(strike, plt.ylim()[0] + (plt.ylim()[1] - plt.ylim()[0]) * 0.02, 
                 f'K={strike:.1f}', rotation=90, verticalalignment='bottom', 
                 horizontalalignment='right', fontsize=8, color='#7f8c8d')

    plt.title(title or f"Strategy Profile: {strategy.get_name()}", fontsize=14, fontweight='bold', pad=15)
    plt.xlabel("Underlying Asset Price (S) at Expiry", fontsize=11, labelpad=10)
    plt.ylabel("Profit / Loss (₹)", fontsize=11, labelpad=10)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(frameon=True, facecolor='white', edgecolor='none')
    
    plt.tight_layout()
    return plt.gcf()


def plot_greek_sensitivity(greek_name: str, variable_name: str, variable_range, 
                            S: float, K: float, r: float, T: float, vol: float, q: float = 0.0, 
                            option_type: str = 'call', title=None):
    """
    Plots the sensitivity of a Greek measure to changes in a specific model parameter.
    For example, Call Delta vs. Underlying Price, or Vega vs. Volatility.

    Parameters:
        greek_name (str): One of 'delta', 'gamma', 'vega', 'theta', 'rho'
        variable_name (str): One of 'underlying_price', 'volatility', 'time_to_maturity' (the variable on the X-axis)
        variable_range (iterable): Values of the variable to plot on the X-axis
        S, K, r, T, vol, q: Reference Black-Scholes parameters
        option_type (str): 'call' or 'put' (for Delta, Theta, Rho which depend on option type)
        title (str, optional): Custom title
    """
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        raise ImportError(
            "The 'matplotlib' and 'numpy' libraries are required for plotting.\n"
            "Install them using: pip install matplotlib numpy\n"
            "Or install with viz dependencies: pip install optipricer[viz]"
        )
        
    from .models import BlackScholesModel, GreeksCalculator
    
    greek = greek_name.lower().strip()
    var = variable_name.lower().strip()
    opt = option_type.lower().strip()
    
    x_vals = np.array(variable_range)
    y_vals = []
    
    for val in x_vals:
        # Construct model with the updated variable value
        curr_S = val if var == 'underlying_price' else S
        curr_vol = val if var == 'volatility' else vol
        curr_T = val if var == 'time_to_maturity' else T
        
        model = BlackScholesModel(strike_price=K, volatility=curr_vol, risk_free_rate=r, 
                                  time_to_maturity=curr_T, underlying_price=curr_S, dividend_yield=q)
        calc = GreeksCalculator(model)
        
        if greek == 'delta':
            y_vals.append(calc.call_delta() if opt == 'call' else calc.put_delta())
        elif greek == 'gamma':
            y_vals.append(calc.gamma())
        elif greek == 'vega':
            y_vals.append(calc.vega())
        elif greek == 'theta':
            y_vals.append(calc.call_theta() if opt == 'call' else calc.put_theta())
        elif greek == 'rho':
            y_vals.append(calc.call_rho() if opt == 'call' else calc.put_rho())
        else:
            raise ValueError(f"Unknown Greek: {greek_name}")
            
    plt.figure(figsize=(10, 6))
    plt.plot(x_vals, y_vals, color='#9b59b6', linewidth=2.5, label=f"{opt.capitalize()} {greek.capitalize()}")
    plt.title(title or f"Greek Analysis: {greek.capitalize()} vs {var.replace('_', ' ').title()}", fontsize=14, fontweight='bold', pad=15)
    plt.xlabel(var.replace('_', ' ').capitalize(), fontsize=11, labelpad=10)
    plt.ylabel(f"Greek Value ({greek.capitalize()})", fontsize=11, labelpad=10)
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(frameon=True, facecolor='white', edgecolor='none')
    
    plt.tight_layout()
    return plt.gcf()


__all__ = ['plot_payoff', 'plot_greek_sensitivity']
