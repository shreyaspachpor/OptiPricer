from ._core.strategies import (
    OptionType,
    PositionType,
    Position,
    OptionsStrategy,
    LongCall,
    ShortCall,
    LongPut,
    ShortPut,
    LongStraddle,
    ShortStraddle,
    LongStrangle,
    ShortStrangle
)

class BullCallSpread(OptionsStrategy):
    """
    Bull Call Spread: Long Call at lower strike, Short Call at higher strike.
    Profits from moderate increases in the underlying asset's price.
    """
    def __init__(self, S: float, sigma: float, r: float, T: float, strike_long: float, strike_short: float, quantity: float = 1.0, q: float = 0.0):
        if strike_long >= strike_short:
            raise ValueError(f"Long strike ({strike_long}) must be less than short strike ({strike_short}) for a Bull Call Spread.")
        
        super().__init__(S, sigma, r, T, "Bull Call Spread", q)
        self.add_position(OptionType.CALL, PositionType.LONG, quantity, strike_long)
        self.add_position(OptionType.CALL, PositionType.SHORT, quantity, strike_short)


class BearPutSpread(OptionsStrategy):
    """
    Bear Put Spread: Long Put at higher strike, Short Put at lower strike.
    Profits from moderate decreases in the underlying asset's price.
    """
    def __init__(self, S: float, sigma: float, r: float, T: float, strike_long: float, strike_short: float, quantity: float = 1.0, q: float = 0.0):
        if strike_long <= strike_short:
            raise ValueError(f"Long strike ({strike_long}) must be greater than short strike ({strike_short}) for a Bear Put Spread.")
        
        super().__init__(S, sigma, r, T, "Bear Put Spread", q)
        self.add_position(OptionType.PUT, PositionType.LONG, quantity, strike_long)
        self.add_position(OptionType.PUT, PositionType.SHORT, quantity, strike_short)


class IronCondor(OptionsStrategy):
    """
    Iron Condor: Long Put (K1), Short Put (K2), Short Call (K3), Long Call (K4).
    Where K1 < K2 < K3 < K4.
    Profits when the underlying stays range-bound between K2 and K3.
    """
    def __init__(self, S: float, sigma: float, r: float, T: float, 
                 put_strike_long: float, put_strike_short: float, 
                 call_strike_short: float, call_strike_long: float, 
                 quantity: float = 1.0, q: float = 0.0):
        if not (put_strike_long < put_strike_short < call_strike_short < call_strike_long):
            raise ValueError(
                f"Iron Condor strikes must satisfy put_strike_long ({put_strike_long}) < "
                f"put_strike_short ({put_strike_short}) < call_strike_short ({call_strike_short}) < "
                f"call_strike_long ({call_strike_long})."
            )
        
        super().__init__(S, sigma, r, T, "Iron Condor", q)
        self.add_position(OptionType.PUT, PositionType.LONG, quantity, put_strike_long)
        self.add_position(OptionType.PUT, PositionType.SHORT, quantity, put_strike_short)
        self.add_position(OptionType.CALL, PositionType.SHORT, quantity, call_strike_short)
        self.add_position(OptionType.CALL, PositionType.LONG, quantity, call_strike_long)


__all__ = [
    'OptionType',
    'PositionType',
    'Position',
    'OptionsStrategy',
    'LongCall',
    'ShortCall',
    'LongPut',
    'ShortPut',
    'LongStraddle',
    'ShortStraddle',
    'LongStrangle',
    'ShortStrangle',
    'BullCallSpread',
    'BearPutSpread',
    'IronCondor'
]
