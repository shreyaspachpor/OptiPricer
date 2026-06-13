import time
import math

try:
    import optipricer
except ImportError:
    print("Warning: optipricer package is not installed. Please compile/install it first (e.g. `pip install -e .`) to run the benchmark.")
    optipricer = None

# --- Pure Python Implementation for Comparison ---

def py_norm_cdf(x):
    return 0.5 * (1.0 + math.erf(x / math.sqrt(2.0)))

def py_norm_pdf(x):
    return (1.0 / math.sqrt(2.0 * math.pi)) * math.exp(-0.5 * x * x)

def py_bs_price(S, K, r, T, vol, q=0.0, is_call=True):
    if T <= 0 or vol <= 0:
        discounted_s = S * math.exp(-q * T)
        discounted_k = K * math.exp(-r * T)
        if is_call:
            return max(discounted_s - discounted_k, 0.0)
        else:
            return max(discounted_k - discounted_s, 0.0)
            
    d1 = (math.log(S / K) + (r - q + 0.5 * vol * vol) * T) / (vol * math.sqrt(T))
    d2 = d1 - vol * math.sqrt(T)
    
    discount_factor = math.exp(-r * T)
    div_discount = math.exp(-q * T)
    
    if is_call:
        return S * div_discount * py_norm_cdf(d1) - K * discount_factor * py_norm_cdf(d2)
    else:
        return K * discount_factor * py_norm_cdf(-d2) - S * div_discount * py_norm_cdf(-d1)

def py_implied_vol(market_price, S, K, r, T, q=0.0, is_call=True, tol=1e-6, max_iter=100):
    discount_factor = math.exp(-r * T)
    div_discount = math.exp(-q * T)
    
    if is_call:
        min_price = max(S * div_discount - K * discount_factor, 0.0)
        max_price = S * div_discount
    else:
        min_price = max(K * discount_factor - S * div_discount, 0.0)
        max_price = K * discount_factor
        
    if market_price < min_price or market_price > max_price:
        raise ValueError("Price is outside bounds.")
        
    low = 1e-10
    high = 10.0
    sigma = 0.5
    
    for i in range(max_iter):
        price = py_bs_price(S, K, r, T, sigma, q, is_call)
        diff = price - market_price
        
        if abs(diff) < tol:
            return sigma
            
        d1_val = (math.log(S / K) + (r - q + 0.5 * sigma * sigma) * T) / (sigma * math.sqrt(T))
        vega = S * div_discount * py_norm_pdf(d1_val) * math.sqrt(T)
        
        sigma_new = -1.0
        if vega > 1e-8:
            sigma_new = sigma - diff / vega
            
        if sigma_new <= low or sigma_new >= high:
            if diff > 0:
                high = sigma
            else:
                low = sigma
            sigma = 0.5 * (low + high)
        else:
            if diff > 0:
                high = sigma
            else:
                low = sigma
            sigma = sigma_new
            
        if abs(high - low) < tol:
            return 0.5 * (low + high)
            
    return sigma

# --- Main Benchmark Suite ---

def run_benchmarks():
    print("=" * 70)
    print("             OPTIPRICER PERFORMANCE BENCHMARK            ")
    print("=" * 70)
    
    S, K, r, T, vol, q = 100.0, 95.0, 0.05, 0.5, 0.25, 0.02
    market_call_price = 9.87
    
    N_pricing = 100000
    N_iv = 10000
    
    # --- 1. Pure Python Pricing ---
    t0 = time.perf_counter()
    for _ in range(N_pricing):
        _ = py_bs_price(S, K, r, T, vol, q, is_call=True)
    t_py_pricing = time.perf_counter() - t0
    
    # --- 2. C++ Pricing: Facade (Instantiates Model every call) ---
    t_cpp_facade = float('nan')
    if optipricer:
        t0 = time.perf_counter()
        for _ in range(N_pricing):
            _ = optipricer.price(S, K, r, T, vol, q, option='call')
        t_cpp_facade = time.perf_counter() - t0
        
    # --- 3. C++ Pricing: OO-Style (Single Instantiation, Method Calls) ---
    t_cpp_oo = float('nan')
    if optipricer:
        model = optipricer.models.BlackScholesModel(strike_price=K, volatility=vol, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q)
        t0 = time.perf_counter()
        for _ in range(N_pricing):
            _ = model.call_price()
        t_cpp_oo = time.perf_counter() - t0

    # --- 4. Pure Python IV Solver ---
    t0 = time.perf_counter()
    for _ in range(N_iv):
        _ = py_implied_vol(market_call_price, S, K, r, T, q, is_call=True)
    t_py_iv = time.perf_counter() - t0
    
    # --- 5. C++ IV Solver (Direct call to Newton-Raphson/Bisection engine) ---
    t_cpp_iv = float('nan')
    if optipricer:
        t0 = time.perf_counter()
        for _ in range(N_iv):
            _ = optipricer.implied_vol(market_call_price, S, K, r, T, q, option='call')
        t_cpp_iv = time.perf_counter() - t0

    # Print results
    print("\n" + "=" * 70)
    print(f"{'Pricing Method (100K iterations)':<32} | {'Time (sec)':<12} | {'Comparison':<16}")
    print("-" * 70)
    print(f"{'Pure Python Function':<32} | {t_py_pricing:.4f}s      | Base (1.0x)")
    if optipricer:
        speed_facade = t_py_pricing / t_cpp_facade
        speed_oo = t_py_pricing / t_cpp_oo
        print(f"{'C++ Facade API (w/ object creation)':<32} | {t_cpp_facade:.4f}s      | {speed_facade:.2f}x speed")
        print(f"{'C++ OO Method (cached object)':<32} | {t_cpp_oo:.4f}s      | {speed_oo:.2f}x speed (Fastest)")
    else:
        print(f"{'C++ Facade API (w/ object creation)':<32} | N/A          | N/A")
        print(f"{'C++ OO Method (cached object)':<32} | N/A          | N/A")
        
    print("\n" + "=" * 70)
    print(f"{'Implied Volatility Solver (10K solves)':<32} | {'Time (sec)':<12} | {'Speedup':<16}")
    print("-" * 70)
    print(f"{'Pure Python Solver':<32} | {t_py_iv:.4f}s      | Base (1.0x)")
    if optipricer:
        speed_iv = t_py_iv / t_cpp_iv
        print(f"{'C++ Solver Engine':<32} | {t_cpp_iv:.4f}s      | {speed_iv:.2f}x speed")
    else:
        print(f"{'C++ Solver Engine':<32} | N/A          | N/A")
        
    print("=" * 70)
    print("Interpretation:")
    print("1. For simple calculations, C++ Method with a cached object is ~2x faster")
    print("   than pure Python, but facade pricing has pybind11 object instantiation overhead.")
    print("2. For iterative numerical routines (like IV solving), C++ outperforms")
    print("   pure Python significantly because the loop runs fully inside C++ compiled code.")
    print("=" * 70)

if __name__ == '__main__':
    run_benchmarks()
