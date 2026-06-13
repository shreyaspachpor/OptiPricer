import os
import optipricer

def main():
    print("=" * 60)
    print("        OPTIPRICER: NSE NIFTY INDEX OPTIONS ANALYSIS DEMO       ")
    print("=" * 60)
    
    # 1. Market Parameters for NIFTY
    # NIFTY spot index is currently at 21,500.
    S = 21500.0
    r = 0.07       # 7% risk free rate (standard for India/NSE)
    q = 0.012      # 1.2% dividend yield (standard NIFTY dividend yield)
    T = 30 / 365.0 # 30 days to expiration (monthly expiry)
    
    # Mock Option Chain strikes and market prices
    # Standard NIFTY option chain has 50-point intervals.
    strikes = [21100, 21200, 21300, 21400, 21500, 21600, 21700, 21800, 21900]
    
    # Call option prices reflecting a typical volatility smile
    # Near the money: volatility is lower, deep in/out of the money: volatility is higher (skew/smile)
    market_calls = [
        540.00,  # 21100 (Deep ITM, intrinsic ~500)
        440.00,  # 21200 (ITM, intrinsic ~400)
        340.00,  # 21300 (ITM, intrinsic ~301)
        240.00,  # 21400 (ITM, intrinsic ~201)
        150.00,  # 21500 (ATM, intrinsic ~102)
        85.00,   # 21600 (OTM, intrinsic ~3)
        45.00,   # 21700 (OTM)
        22.00,   # 21800 (OTM)
        10.00    # 21900 (OTM)
    ]
    
    print(f"NIFTY Spot Index: INR {S:.2f}")
    print(f"Days to Expiry: {T * 365.0:.0f} days")
    print(f"Risk-free Rate: {r*100:.1f}%, Dividend Yield: {q*100:.2f}%")
    print("\nSolving Implied Volatilities for Calls:")
    print("-" * 50)
    print(f"{'Strike (K)':<12} | {'Market Price':<14} | {'Implied Volatility (IV)':<20}")
    print("-" * 50)
    
    ivs = []
    for strike, market_price in zip(strikes, market_calls):
        # Calculate IV using the high-performance C++ hybrid solver
        iv = optipricer.implied_vol(
            market_price=market_price,
            S=S,
            K=strike,
            r=r,
            T=T,
            q=q,
            option='call'
        )
        ivs.append(iv)
        print(f"INR {strike:<7,d} | INR {market_price:<10.2f} | {iv * 100:.2f}%")
        
    print("-" * 50)
    
    # Check if matplotlib is available to plot the results
    try:
        import matplotlib.pyplot as plt
        
        # Save plots under examples directory
        os.makedirs("examples", exist_ok=True)
        
        # Plot 1: Volatility Smile
        plt.figure(figsize=(9, 5.5))
        plt.plot(strikes, [v * 100 for v in ivs], marker='o', color='#8e44ad', linewidth=2.5, markersize=8)
        plt.title("NIFTY Volatility Smile (30-day Expiry)", fontsize=13, fontweight='bold', pad=12)
        plt.xlabel("Strike Price (₹)", fontsize=11, labelpad=8)
        plt.ylabel("Implied Volatility (IV %)", fontsize=11, labelpad=8)
        plt.grid(True, linestyle=':', alpha=0.6)
        
        # Highlight Spot price
        plt.axvline(S, color='#f39c12', linestyle='--', label=f'NIFTY Spot (₹{S:,.0f})')
        plt.legend(frameon=True, facecolor='white', edgecolor='none')
        
        smile_path = "examples/volatility_smile.png"
        plt.savefig(smile_path, dpi=150)
        print(f"\n[Success] Volatility Smile plot saved to: {smile_path}")
        plt.close()
        
        # 2. Strategy Analysis: Iron Condor
        # Set up a NIFTY Iron Condor strategy:
        # Buy Put 21100, Sell Put 21300, Sell Call 21700, Buy Call 21900
        print("\nCreating a NIFTY Iron Condor Strategy:")
        ic = optipricer.strategies.IronCondor(
            S=S,
            sigma=0.16,      # Average implied volatility (16%)
            r=r,
            T=T,
            put_strike_long=21100,
            put_strike_short=21300,
            call_strike_short=21700,
            call_strike_long=21900,
            quantity=1.0,
            q=q
        )
        
        # Contract information using NSE lot size
        lot_size = 50
        ic_value = ic.total_value()
        net_credit = -ic_value  # Net credit strategy
        max_profit = net_credit * lot_size
        
        print(f"Strategy Name: {ic.get_name()}")
        print(f"Strategy Value (Premium Net Debit/Credit): INR {ic_value:.2f}")
        print(f"Net Premium Received (per share): INR {net_credit:.2f}")
        print(f"NIFTY Lot Size: {lot_size}")
        print(f"Max Profit (at expiry): INR {max_profit:.2f}")
        print(f"Portfolio Delta: {ic.total_delta():.4f}")
        
        # Plot 2: Payoff Profile
        # Evaluate over a range of NIFTY index values
        nifty_range = range(20800, 22200, 5)
        
        # Let's use our package viz helper to plot it!
        optipricer.viz.plot_payoff(
            strategy=ic,
            underlying_range=nifty_range,
            title="NIFTY Iron Condor Expiration Payoff (Lot Size=50)"
        )
        
        payoff_path = "examples/iron_condor_payoff.png"
        plt.savefig(payoff_path, dpi=150)
        print(f"[Success] Iron Condor Payoff plot saved to: {payoff_path}")
        plt.close()
        
    except ImportError:
        print("\nNote: matplotlib is not installed. Skipping plot generation.")
        print("Install it with: pip install matplotlib to enable beautiful visual plots.")
        
    print("\n" + "=" * 60)
    print("Demo execution completed successfully!")
    print("=" * 60)

if __name__ == '__main__':
    main()
