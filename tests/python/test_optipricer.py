import pytest
import optipricer
import math

def test_black_scholes_validation():
    # Test valid parameters
    model = optipricer.models.BlackScholesModel(
        strike_price=100.0,
        volatility=0.2,
        risk_free_rate=0.05,
        time_to_maturity=1.0,
        underlying_price=100.0
    )
    assert model.get_strike_price() == 100.0
    assert model.get_volatility() == 0.2
    assert model.get_risk_free_rate() == 0.05
    assert model.get_time_to_maturity() == 1.0
    assert model.get_underlying_price() == 100.0

    # Test invalid strike price
    with pytest.raises(ValueError, match="Strike price must be positive"):
        optipricer.models.BlackScholesModel(0.0, 0.2, 0.05, 1.0, 100.0)
    with pytest.raises(ValueError, match="Strike price must be positive"):
        optipricer.models.BlackScholesModel(-10.0, 0.2, 0.05, 1.0, 100.0)

    # Test invalid volatility
    with pytest.raises(ValueError, match="Volatility must be non-negative"):
        optipricer.models.BlackScholesModel(100.0, -0.1, 0.05, 1.0, 100.0)
    with pytest.raises(ValueError, match="Volatility seems unreasonably high"):
        optipricer.models.BlackScholesModel(100.0, 11.0, 0.05, 1.0, 100.0)

    # Test invalid time to maturity
    with pytest.raises(ValueError, match="Time to maturity must be positive"):
        optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 0.0, 100.0)
    with pytest.raises(ValueError, match="Time to maturity must be positive"):
        optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, -0.5, 100.0)
    with pytest.raises(ValueError, match="Time to maturity seems unreasonably high"):
        optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 101.0, 100.0)

    # Test invalid underlying price
    with pytest.raises(ValueError, match="Underlying price must be positive"):
        optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 1.0, 0.0)
    with pytest.raises(ValueError, match="Underlying price must be positive"):
        optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 1.0, -50.0)

def test_black_scholes_pricing():
    # Standard values
    model = optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 1.0, 100.0)
    call = model.call_price()
    put = model.put_price()
    # Call price should be ~10.45058, Put price should be ~5.57352
    assert abs(call - 10.45058) < 1e-4
    assert abs(put - 5.57352) < 1e-4

    # Put-Call Parity: C - P = S - K * exp(-r * T)
    discounted_k = 100.0 * (2.718281828459045 ** -0.05)
    assert (call - put) == pytest.approx(100.0 - discounted_k)

def test_greeks_numerical():
    # S=100, K=100, r=0.05, T=1.0, vol=0.2
    S = 100.0
    K = 100.0
    r = 0.05
    T = 1.0
    vol = 0.2

    model = optipricer.models.BlackScholesModel(K, vol, r, T, S)
    greeks = optipricer.models.GreeksCalculator(model)

    # Helper function to get price for variable inputs
    def get_prices(S_val, vol_val, r_val, T_val):
        m = optipricer.models.BlackScholesModel(K, vol_val, r_val, T_val, S_val)
        return m.call_price(), m.put_price()

    # Finite difference step sizes
    dS = 1e-5
    dvol = 1e-5
    dr = 1e-5
    dT = 1e-5

    # 1. Delta: dPrice / dS
    c_plus, p_plus = get_prices(S + dS, vol, r, T)
    c_minus, p_minus = get_prices(S - dS, vol, r, T)
    num_call_delta = (c_plus - c_minus) / (2 * dS)
    num_put_delta = (p_plus - p_minus) / (2 * dS)
    
    assert greeks.call_delta() == pytest.approx(num_call_delta, rel=1e-5)
    assert greeks.put_delta() == pytest.approx(num_put_delta, rel=1e-5)

    # 2. Gamma: d^2Price / dS^2
    c_center, _ = get_prices(S, vol, r, T)
    dS_gamma = 1e-3
    c_plus_g, _ = get_prices(S + dS_gamma, vol, r, T)
    c_minus_g, _ = get_prices(S - dS_gamma, vol, r, T)
    num_gamma = (c_plus_g - 2 * c_center + c_minus_g) / (dS_gamma ** 2)
    assert greeks.gamma() == pytest.approx(num_gamma, rel=1e-4)

    # 3. Vega: dPrice / dvol (divided by 100 for 1% vol change)
    c_vol_plus, p_vol_plus = get_prices(S, vol + dvol, r, T)
    c_vol_minus, p_vol_minus = get_prices(S, vol - dvol, r, T)
    num_vega = ((c_vol_plus - c_vol_minus) / (2 * dvol)) / 100.0
    assert greeks.vega() == pytest.approx(num_vega, rel=1e-5)

    # 4. Theta: -dPrice / dT (per day, divided by 365)
    # Note: Theta is defined as change per unit of time passing (so T decreasing)
    c_T_plus, p_T_plus = get_prices(S, vol, r, T + dT)
    c_T_minus, p_T_minus = get_prices(S, vol, r, T - dT)
    # Theta is dPrice/dt = -dPrice/dT. To convert to per-day: (dPrice/dT) / 365
    num_call_theta = ((c_T_plus - c_T_minus) / (2 * dT)) / 365.0
    num_put_theta = ((p_T_plus - p_T_minus) / (2 * dT)) / 365.0
    # Wait, let's verify if the sign is correct.
    # Usually Theta is negative because option loses value over time, but the definition is:
    # dPrice / dt where dt is calendar time (i.e. T decreases). So Theta = -dPrice/dT.
    # Let's check what the sign is in the code.
    # In C++ code: term1 = -(S * pdf * sigma)/(2*sqrt(T)) and term2 = -r * K * exp(-rT) * N(d2).
    # Since term1 and term2 are negative, call_theta is negative.
    # So call_theta matches -dPrice/dT.
    # Let's compare with -num_call_theta
    assert greeks.call_theta() == pytest.approx(-num_call_theta, rel=1e-5)
    assert greeks.put_theta() == pytest.approx(-num_put_theta, rel=1e-5)

    # 5. Rho: dPrice / dr (divided by 100 for 1% rate change)
    c_r_plus, p_r_plus = get_prices(S, vol, r + dr, T)
    c_r_minus, p_r_minus = get_prices(S, vol, r - dr, T)
    num_call_rho = ((c_r_plus - c_r_minus) / (2 * dr)) / 100.0
    num_put_rho = ((p_r_plus - p_r_minus) / (2 * dr)) / 100.0
    assert greeks.call_rho() == pytest.approx(num_call_rho, rel=1e-5)
    assert greeks.put_rho() == pytest.approx(num_put_rho, rel=1e-5)

def test_strategies():
    # Test LongStraddle
    straddle = optipricer.strategies.LongStraddle(100.0, 0.2, 0.05, 1.0, 100.0, 1.0)
    # Total value should be call_price + put_price
    model = optipricer.models.BlackScholesModel(100.0, 0.2, 0.05, 1.0, 100.0)
    expected_val = model.call_price() + model.put_price()
    assert abs(straddle.total_value() - expected_val) < 1e-7

    # Test payoff at expiration
    assert straddle.payoff_at_expiration(120.0) == pytest.approx(20.0)
    assert straddle.payoff_at_expiration(80.0) == pytest.approx(20.0)
    assert straddle.payoff_at_expiration(100.0) == pytest.approx(0.0)

def test_edge_cases_pricing():
    # ITM Call under very small volatility
    model_call_itm = optipricer.models.BlackScholesModel(
        strike_price=95.0, volatility=1e-12, risk_free_rate=0.05, time_to_maturity=1e-12, underlying_price=100.0
    )
    assert model_call_itm.call_price() == pytest.approx(5.0)
    assert model_call_itm.put_price() == pytest.approx(0.0)

    # OTM Call under very small volatility
    model_call_otm = optipricer.models.BlackScholesModel(
        strike_price=105.0, volatility=1e-12, risk_free_rate=0.05, time_to_maturity=1e-12, underlying_price=100.0
    )
    assert model_call_otm.call_price() == pytest.approx(0.0)
    assert model_call_otm.put_price() == pytest.approx(5.0)

def test_edge_cases_greeks():
    # ITM Call
    model_itm = optipricer.models.BlackScholesModel(
        strike_price=95.0, volatility=1e-12, risk_free_rate=0.05, time_to_maturity=1e-12, underlying_price=100.0
    )
    greeks_itm = optipricer.models.GreeksCalculator(model_itm)
    assert greeks_itm.call_delta() == pytest.approx(1.0)
    assert greeks_itm.put_delta() == pytest.approx(0.0)
    assert greeks_itm.gamma() == pytest.approx(0.0)
    assert greeks_itm.vega() == pytest.approx(0.0)
    assert greeks_itm.call_theta() == pytest.approx(-0.05 * 95.0 / 365.0)
    assert greeks_itm.put_theta() == pytest.approx(0.0)

    # OTM Call
    model_otm = optipricer.models.BlackScholesModel(
        strike_price=105.0, volatility=1e-12, risk_free_rate=0.05, time_to_maturity=1e-12, underlying_price=100.0
    )
    greeks_otm = optipricer.models.GreeksCalculator(model_otm)
    assert greeks_otm.call_delta() == pytest.approx(0.0)
    assert greeks_otm.put_delta() == pytest.approx(-1.0)
    assert greeks_otm.gamma() == pytest.approx(0.0)
    assert greeks_otm.vega() == pytest.approx(0.0)
    assert greeks_otm.call_theta() == pytest.approx(0.0)
    assert greeks_otm.put_theta() == pytest.approx(0.05 * 105.0 / 365.0)

def test_strategies_validation():
    # Constructor validation
    with pytest.raises(ValueError, match="Underlying price must be positive"):
        optipricer.strategies.LongCall(underlying_price=-10.0, volatility=0.2, risk_free_rate=0.05, time_to_maturity=1.0, strike=100.0)
    
    with pytest.raises(ValueError, match="Volatility must be non-negative"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=-0.1, risk_free_rate=0.05, time_to_maturity=1.0, strike=100.0)

    with pytest.raises(ValueError, match="Volatility seems unreasonably high"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=11.0, risk_free_rate=0.05, time_to_maturity=1.0, strike=100.0)

    with pytest.raises(ValueError, match="Time to maturity must be positive"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=0.2, risk_free_rate=0.05, time_to_maturity=0.0, strike=100.0)

    with pytest.raises(ValueError, match="Time to maturity seems unreasonably high"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=0.2, risk_free_rate=0.05, time_to_maturity=101.0, strike=100.0)

    # Position validation inside constructor (strike/qty validation)
    with pytest.raises(ValueError, match="Strike price must be positive"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=0.2, risk_free_rate=0.05, time_to_maturity=1.0, strike=-5.0)

    with pytest.raises(ValueError, match="Quantity must be positive"):
        optipricer.strategies.LongCall(underlying_price=100.0, volatility=0.2, risk_free_rate=0.05, time_to_maturity=1.0, strike=100.0, quantity=-2.0)

    # payoff_at_expiration validation
    straddle = optipricer.strategies.LongStraddle(100.0, 0.2, 0.05, 1.0, 100.0)
    with pytest.raises(ValueError, match="Stock price at expiration must be non-negative"):
        straddle.payoff_at_expiration(-5.0)

def test_dividend_yield_pricing_and_greeks():
    S = 100.0
    K = 100.0
    r = 0.05
    T = 1.0
    vol = 0.2
    q = 0.03

    model_with_q = optipricer.models.BlackScholesModel(K, vol, r, T, S, q)
    model_no_q = optipricer.models.BlackScholesModel(K, vol, r, T, S, 0.0)

    call_q = model_with_q.call_price()
    put_q = model_with_q.put_price()
    call_no_q = model_no_q.call_price()
    put_no_q = model_no_q.put_price()

    # Dividend decreases call price and increases put price
    assert call_q < call_no_q
    assert put_q > put_no_q

    # Put-Call Parity with dividend: C - P = S * exp(-q * T) - K * exp(-r * T)
    discounted_s = S * math.exp(-q * T)
    discounted_k = K * math.exp(-r * T)
    assert (call_q - put_q) == pytest.approx(discounted_s - discounted_k)

    # Greeks with dividend yield
    greeks_q = optipricer.models.GreeksCalculator(model_with_q)
    
    # Delta check: call_delta - put_delta = exp(-q * T)
    assert (greeks_q.call_delta() - greeks_q.put_delta()) == pytest.approx(math.exp(-q * T))

def test_implied_volatility_solver():
    S = 100.0
    K = 95.0
    r = 0.05
    T = 0.5
    vol_target = 0.25
    q = 0.02

    # Call
    model_call = optipricer.models.BlackScholesModel(K, vol_target, r, T, S, q)
    call_price = model_call.call_price()

    iv_call = optipricer.models.calculate_implied_volatility(
        market_price=call_price, strike_price=K, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q, is_call=True
    )
    assert iv_call == pytest.approx(vol_target, abs=1e-5)

    # Put
    model_put = optipricer.models.BlackScholesModel(K, vol_target, r, T, S, q)
    put_price = model_put.put_price()

    iv_put = optipricer.models.calculate_implied_volatility(
        market_price=put_price, strike_price=K, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q, is_call=False
    )
    assert iv_put == pytest.approx(vol_target, abs=1e-5)

    # Bounds check
    # Price below minimum price
    with pytest.raises(ValueError, match="is outside theoretical Black-Scholes bounds"):
        optipricer.models.calculate_implied_volatility(
            market_price=0.01, strike_price=K, risk_free_rate=r, time_to_maturity=T, underlying_price=S, dividend_yield=q, is_call=True
        )


def test_high_level_facade_api():
    # S=100, K=105, r=0.05, T=0.25, vol=0.25, q=0.03
    S, K, r, T, vol, q = 100.0, 105.0, 0.05, 0.25, 0.25, 0.03
    
    # Test price helper
    c_p = optipricer.price(S, K, r, T, vol, q, option='call')
    p_p = optipricer.price(S, K, r, T, vol, q, option='put')
    
    model = optipricer.models.BlackScholesModel(K, vol, r, T, S, q)
    assert c_p == pytest.approx(model.call_price())
    assert p_p == pytest.approx(model.put_price())
    
    # Test invalid option type in price
    with pytest.raises(ValueError, match="Invalid option type"):
        optipricer.price(S, K, r, T, vol, q, option='invalid')
        
    # Test greeks helper
    gr = optipricer.greeks(S, K, r, T, vol, q)
    calc = optipricer.models.GreeksCalculator(model)
    assert gr['call_delta'] == pytest.approx(calc.call_delta())
    assert gr['put_delta'] == pytest.approx(calc.put_delta())
    assert gr['gamma'] == pytest.approx(calc.gamma())
    assert gr['vega'] == pytest.approx(calc.vega())
    assert gr['call_theta'] == pytest.approx(calc.call_theta())
    assert gr['put_theta'] == pytest.approx(calc.put_theta())
    assert gr['call_rho'] == pytest.approx(calc.call_rho())
    assert gr['put_rho'] == pytest.approx(calc.put_rho())
    
    # Test implied_vol helper
    solved_iv = optipricer.implied_vol(c_p, S, K, r, T, q, option='call')
    assert solved_iv == pytest.approx(vol, abs=1e-5)
    
    with pytest.raises(ValueError, match="Invalid option type"):
        optipricer.implied_vol(c_p, S, K, r, T, q, option='invalid')


def test_representations():
    model = optipricer.models.BlackScholesModel(105.0, 0.25, 0.05, 0.25, 100.0, 0.03)
    calc = optipricer.models.GreeksCalculator(model)
    
    model_repr = repr(model)
    assert "BlackScholesModel" in model_repr
    assert "strike_price=105" in model_repr
    assert "underlying_price=100" in model_repr
    assert "dividend_yield=0.03" in model_repr
    
    calc_repr = repr(calc)
    assert "GreeksCalculator" in calc_repr
    assert "S=100" in calc_repr
    assert "K=105" in calc_repr
    
    strategy = optipricer.strategies.LongStraddle(100.0, 0.25, 0.05, 0.25, 100.0)
    strat_repr = repr(strategy)
    assert "OptionsStrategy" in strat_repr
    assert 'name="Long Straddle"' in strat_repr
    assert "positions=[" in strat_repr
    assert "Position(CALL, LONG" in strat_repr
    assert "Position(PUT, LONG" in strat_repr
    
    positions = strategy.get_positions()
    assert len(positions) == 2
    pos_repr = repr(positions[0])
    assert "Position" in pos_repr
    assert "option_type=OptionType.CALL" in pos_repr
    assert "position_type=PositionType.LONG" in pos_repr
    assert "strike=100" in pos_repr


def test_python_strategies():
    # BullCallSpread: Buy Call 100, Sell Call 105
    S, sigma, r, T, q = 100.0, 0.2, 0.05, 1.0, 0.0
    spread = optipricer.strategies.BullCallSpread(S, sigma, r, T, 100.0, 105.0)
    assert spread.get_name() == "Bull Call Spread"
    assert len(spread.get_positions()) == 2
    
    # Payoffs
    assert spread.payoff_at_expiration(95.0) == pytest.approx(0.0)
    assert spread.payoff_at_expiration(102.5) == pytest.approx(2.5)
    assert spread.payoff_at_expiration(110.0) == pytest.approx(5.0)
    
    # Strike checks
    with pytest.raises(ValueError, match="must be less than short strike"):
        optipricer.strategies.BullCallSpread(S, sigma, r, T, 105.0, 100.0)
        
    # BearPutSpread: Buy Put 105, Sell Put 100
    bear = optipricer.strategies.BearPutSpread(S, sigma, r, T, 105.0, 100.0)
    assert bear.get_name() == "Bear Put Spread"
    assert len(bear.get_positions()) == 2
    assert bear.payoff_at_expiration(110.0) == pytest.approx(0.0)
    assert bear.payoff_at_expiration(102.5) == pytest.approx(2.5)
    assert bear.payoff_at_expiration(95.0) == pytest.approx(5.0)
    
    with pytest.raises(ValueError, match="must be greater than short strike"):
        optipricer.strategies.BearPutSpread(S, sigma, r, T, 100.0, 105.0)
        
    # IronCondor: Put K1=90 (Long), Put K2=95 (Short), Call K3=105 (Short), Call K4=110 (Long)
    condor = optipricer.strategies.IronCondor(S, sigma, r, T, 90.0, 95.0, 105.0, 110.0)
    assert condor.get_name() == "Iron Condor"
    assert len(condor.get_positions()) == 4
    
    # Payoffs: net payoff (gross)
    assert condor.payoff_at_expiration(85.0) == pytest.approx(-5.0)  # Put spread max loss
    assert condor.payoff_at_expiration(92.5) == pytest.approx(-2.5)
    assert condor.payoff_at_expiration(100.0) == pytest.approx(0.0)
    assert condor.payoff_at_expiration(107.5) == pytest.approx(-2.5)
    assert condor.payoff_at_expiration(115.0) == pytest.approx(-5.0)  # Call spread max loss
    
    with pytest.raises(ValueError, match="strikes must satisfy"):
        optipricer.strategies.IronCondor(S, sigma, r, T, 95.0, 90.0, 105.0, 110.0)


def test_nse_helpers():
    # Round to tick (default 0.05)
    assert optipricer.nse.round_to_tick(100.02) == pytest.approx(100.0)
    assert optipricer.nse.round_to_tick(100.03) == pytest.approx(100.05)
    assert optipricer.nse.round_to_tick(100.076) == pytest.approx(100.10)
    
    # Custom tick sizes
    assert optipricer.nse.round_to_tick(100.12, tick=0.1) == pytest.approx(100.10)
    
    with pytest.raises(ValueError, match="must be strictly positive"):
        optipricer.nse.round_to_tick(100.0, tick=-0.05)
        
    # Contract value helper
    S, sigma, r, T = 100.0, 0.25, 0.05, 0.25
    strategy = optipricer.strategies.LongCall(S, sigma, r, T, 100.0)
    single_val = strategy.total_value()
    assert optipricer.nse.contract_value(strategy, lot_size=50) == pytest.approx(single_val * 50)
    
    with pytest.raises(ValueError, match="must be strictly positive"):
        optipricer.nse.contract_value(strategy, lot_size=0)


def test_viz_smoke():
    # If matplotlib is installed, verify plotting runs without errors
    try:
        import matplotlib
        matplotlib.use('Agg') # Use headless backend for testing
        
        S, sigma, r, T = 100.0, 0.25, 0.05, 0.25
        strategy = optipricer.strategies.LongCall(S, sigma, r, T, 100.0)
        
        fig = optipricer.viz.plot_payoff(strategy)
        assert fig is not None
        
        fig2 = optipricer.viz.plot_greek_sensitivity(
            greek_name='delta',
            variable_name='underlying_price',
            variable_range=[80, 90, 100, 110, 120],
            S=S, K=100.0, r=r, T=T, vol=sigma
        )
        assert fig2 is not None
    except ImportError:
        # If not installed, calling these should raise ImportError as expected
        S, sigma, r, T = 100.0, 0.25, 0.05, 0.25
        strategy = optipricer.strategies.LongCall(S, sigma, r, T, 100.0)
        with pytest.raises(ImportError):
            optipricer.viz.plot_payoff(strategy)


def test_second_order_greeks():
    """Test Vanna, Volga, and Charm via finite difference verification."""
    S = 100.0
    K = 110.0  # OTM — avoids d2≈0 at ATM which makes vanna near-zero
    r = 0.05
    T = 1.0
    vol = 0.2
    q = 0.03

    model = optipricer.models.BlackScholesModel(K, vol, r, T, S, q)
    greeks = optipricer.models.GreeksCalculator(model)

    def get_delta(S_val, vol_val, r_val, T_val):
        m = optipricer.models.BlackScholesModel(K, vol_val, r_val, T_val, S_val, q)
        c = optipricer.models.GreeksCalculator(m)
        return c.call_delta()

    def get_vega_raw(S_val, vol_val, r_val, T_val):
        m = optipricer.models.BlackScholesModel(K, vol_val, r_val, T_val, S_val, q)
        c = optipricer.models.GreeksCalculator(m)
        # Raw vega (unscaled) = vega * 100
        return c.vega() * 100.0

    # 1. Vanna: dDelta/dVol (via finite difference on delta w.r.t. vol)
    dvol = 1e-5
    delta_plus = get_delta(S, vol + dvol, r, T)
    delta_minus = get_delta(S, vol - dvol, r, T)
    num_vanna = (delta_plus - delta_minus) / (2 * dvol)
    assert greeks.vanna() == pytest.approx(num_vanna, rel=1e-4)

    # 2. Volga: dVega/dVol (via finite difference on raw vega w.r.t. vol)
    vega_plus = get_vega_raw(S, vol + dvol, r, T)
    vega_minus = get_vega_raw(S, vol - dvol, r, T)
    num_volga = (vega_plus - vega_minus) / (2 * dvol)
    assert greeks.volga() == pytest.approx(num_volga, rel=1e-4)

    # 3. Charm: dDelta/dT (via finite difference, per day)
    dT = 1e-5
    delta_T_plus = get_delta(S, vol, r, T + dT)
    delta_T_minus = get_delta(S, vol, r, T - dT)
    # Charm = -dDelta/dT / 365 (our implementation divides by 365)
    num_call_charm = -(delta_T_plus - delta_T_minus) / (2 * dT) / 365.0
    assert greeks.call_charm() == pytest.approx(num_call_charm, rel=1e-4)

    # Edge case: very small vol/T should return 0
    model_edge = optipricer.models.BlackScholesModel(K, 1e-12, r, 1e-12, S, q)
    greeks_edge = optipricer.models.GreeksCalculator(model_edge)
    assert greeks_edge.vanna() == pytest.approx(0.0)
    assert greeks_edge.volga() == pytest.approx(0.0)
    assert greeks_edge.call_charm() == pytest.approx(0.0)
    assert greeks_edge.put_charm() == pytest.approx(0.0)


def test_strategy_full_greeks():
    """Test that strategy-level Greeks (gamma, vega, theta, rho) are properly aggregated."""
    S, sigma, r, T, q = 100.0, 0.2, 0.05, 1.0, 0.0

    # LongCall: strategy greeks should equal individual option greeks
    call = optipricer.strategies.LongCall(S, sigma, r, T, 100.0, 1.0)
    model = optipricer.models.BlackScholesModel(100.0, sigma, r, T, S, q)
    calc = optipricer.models.GreeksCalculator(model)

    assert call.total_delta() == pytest.approx(calc.call_delta())
    assert call.total_gamma() == pytest.approx(calc.gamma())
    assert call.total_vega() == pytest.approx(calc.vega())
    assert call.total_theta() == pytest.approx(calc.call_theta())
    assert call.total_rho() == pytest.approx(calc.call_rho())
    assert call.total_vanna() == pytest.approx(calc.vanna())
    assert call.total_volga() == pytest.approx(calc.volga())
    assert call.total_charm() == pytest.approx(calc.call_charm())

    # LongStraddle: gamma should be 2x single option gamma (call and put gamma are equal)
    straddle = optipricer.strategies.LongStraddle(S, sigma, r, T, 100.0, 1.0)
    assert straddle.total_gamma() == pytest.approx(2.0 * calc.gamma())
    assert straddle.total_vega() == pytest.approx(2.0 * calc.vega())

    # IronCondor: should have near-zero delta (it's delta-neutral-ish)
    condor = optipricer.strategies.IronCondor(S, sigma, r, T, 90.0, 95.0, 105.0, 110.0)
    # Iron condor is approximately delta neutral
    assert abs(condor.total_delta()) < 0.1  
    # Iron condor has negative gamma (short gamma position)
    assert condor.total_gamma() < 0.0
    # Iron condor has positive theta (collects time decay)
    assert condor.total_theta() > 0.0


def test_option_chain():
    """Test the OptionChain builder."""
    from optipricer.chain import OptionChain, generate_nifty_strikes

    # Basic chain construction
    strikes = [95.0, 97.5, 100.0, 102.5, 105.0]
    chain = OptionChain(S=100.0, r=0.05, T=0.25, strikes=strikes, vol=0.20)
    
    data = chain.build()
    assert len(data) == 5
    
    # Each row should have all expected keys
    expected_keys = {'strike', 'iv', 'call_price', 'put_price',
                     'call_delta', 'put_delta', 'gamma', 'vega',
                     'call_theta', 'put_theta', 'call_rho', 'put_rho',
                     'vanna', 'volga', 'call_charm', 'put_charm'}
    for row in data:
        assert set(row.keys()) == expected_keys

    # ATM strike should be 100.0
    assert chain.atm_strike() == 100.0

    # Summary
    summary = chain.summary()
    assert summary['underlying'] == 100.0
    assert summary['atm_strike'] == 100.0
    assert summary['num_strikes'] == 5

    # to_dict should return same data
    assert chain.to_dict() == data

    # Repr
    r = repr(chain)
    assert 'OptionChain' in r

    # generate_nifty_strikes
    nifty_strikes = generate_nifty_strikes(21500.0, step=50.0, count=11)
    assert len(nifty_strikes) == 11
    # Center should be near 21500
    assert 21500.0 in nifty_strikes

    # iv_map override
    iv_map = {100.0: 0.30, 105.0: 0.25}
    chain2 = OptionChain(S=100.0, r=0.05, T=0.25, strikes=[100.0, 105.0], vol=0.20, iv_map=iv_map)
    data2 = chain2.build()
    assert data2[0]['iv'] == 0.30  # Overridden
    assert data2[1]['iv'] == 0.25  # Overridden

    # Empty strikes should raise
    with pytest.raises(ValueError, match="must not be empty"):
        OptionChain(S=100.0, r=0.05, T=0.25, strikes=[], vol=0.20)


def test_volatility_surface():
    """Test the VolatilitySurface class."""
    from optipricer.surface import VolatilitySurface

    strikes = [95.0, 100.0, 105.0]
    expiries = [0.08, 0.25]  # ~1 month, ~3 months
    iv_matrix = [
        [0.22, 0.20, 0.21],   # Near-term smile
        [0.21, 0.19, 0.20],   # Far-term (slightly lower)
    ]

    surface = VolatilitySurface(strikes, expiries, iv_matrix)

    # Exact grid points should return exact values
    assert surface.get_iv(95.0, 0.08) == pytest.approx(0.22)
    assert surface.get_iv(100.0, 0.25) == pytest.approx(0.19)
    assert surface.get_iv(105.0, 0.08) == pytest.approx(0.21)

    # Interpolated point (midpoint of strike and expiry)
    iv_mid = surface.get_iv(100.0, 0.165)  # Midpoint of expiries
    # Should be between 0.20 and 0.19
    assert 0.19 <= iv_mid <= 0.20

    # Smile extraction
    smile_strikes, smile_ivs = surface.smile(0)
    assert len(smile_strikes) == 3
    assert smile_ivs[1] == 0.20  # ATM point

    # Term structure
    term_expiries, term_ivs = surface.term_structure(100.0)
    assert len(term_expiries) == 2
    assert term_ivs[0] == 0.20
    assert term_ivs[1] == 0.19

    # from_chain_data
    chain_data = [
        {'expiry': 0.08, 'strikes': [95.0, 100.0, 105.0], 'ivs': [0.22, 0.20, 0.21]},
        {'expiry': 0.25, 'strikes': [95.0, 100.0, 105.0], 'ivs': [0.21, 0.19, 0.20]},
    ]
    surface2 = VolatilitySurface.from_chain_data(chain_data)
    assert surface2.get_iv(100.0, 0.08) == pytest.approx(0.20)

    # Repr
    r = repr(surface)
    assert 'VolatilitySurface' in r

    # Validation
    with pytest.raises(ValueError, match="non-empty"):
        VolatilitySurface([], [0.08], [[0.2]])
    
    with pytest.raises(ValueError, match="non-empty"):
        VolatilitySurface([100.0], [], [])

    with pytest.raises(ValueError):
        VolatilitySurface([100.0], [0.08, 0.25], [[0.2]])  # Wrong number of rows

    with pytest.raises(ValueError):
        VolatilitySurface([95.0, 100.0], [0.08], [[0.2]])  # Wrong number of columns


def test_facade_greeks_second_order():
    """Test that the facade greeks() function includes 2nd-order Greeks."""
    S, K, r, T, vol, q = 100.0, 105.0, 0.05, 0.25, 0.25, 0.03
    g = optipricer.greeks(S, K, r, T, vol, q)

    # Check all keys present
    assert 'vanna' in g
    assert 'volga' in g
    assert 'call_charm' in g
    assert 'put_charm' in g

    # Verify against direct calculation
    model = optipricer.models.BlackScholesModel(K, vol, r, T, S, q)
    calc = optipricer.models.GreeksCalculator(model)
    assert g['vanna'] == pytest.approx(calc.vanna())
    assert g['volga'] == pytest.approx(calc.volga())
    assert g['call_charm'] == pytest.approx(calc.call_charm())
    assert g['put_charm'] == pytest.approx(calc.put_charm())


def test_surface_plot_smoke():
    """Test VolatilitySurface plotting (smoke test)."""
    try:
        import matplotlib
        matplotlib.use('Agg')

        from optipricer.surface import VolatilitySurface

        strikes = [95.0, 100.0, 105.0]
        expiries = [0.08, 0.25]
        iv_matrix = [[0.22, 0.20, 0.21], [0.21, 0.19, 0.20]]
        
        surface = VolatilitySurface(strikes, expiries, iv_matrix)
        fig = surface.plot()
        assert fig is not None
        
        fig2 = surface.plot_smile(0)
        assert fig2 is not None
    except ImportError:
        pytest.skip("matplotlib not installed")
