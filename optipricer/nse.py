def round_to_tick(price: float, tick: float = 0.05) -> float:
    """
    Rounds a given price to the nearest tick size.
    The default tick size is 0.05, which is standard for options traded on the NSE (India).

    Parameters:
        price (float): The price to round (e.g. option price or underlying price)
        tick (float): The tick size constraint (default 0.05)

    Returns:
        float: Rounded price
    """
    if tick <= 0:
        raise ValueError("Tick size must be strictly positive.")
    return round(price / tick) * tick

def contract_value(strategy, lot_size: int = 50) -> float:
    """
    Calculates the total monetary value of an options strategy based on the contract lot size.
    For NIFTY index options, the standard lot size is 50.

    Parameters:
        strategy: An instance of optipricer.strategies.OptionsStrategy
        lot_size (int): The number of shares per option contract (default 50)

    Returns:
        float: Total monetary value of the strategy in Rupee (or the base currency)
    """
    if lot_size <= 0:
        raise ValueError("Lot size must be strictly positive.")
    return strategy.total_value() * lot_size

__all__ = ['round_to_tick', 'contract_value']
