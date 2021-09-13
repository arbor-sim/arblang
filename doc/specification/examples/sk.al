# Adapted from Allen Institute SK.mod, in turn based on model of Kohler et al. 1996.
# Takes fixed ek as parameter.

interface density "SK" {
    export density parameter gbar = 10^-5 S/cm^2;
    export parameter zTau = 1 ms;

    def zInf(v: voltage; c: concentration) ->
        | c==0 -> 0
        | otherwise -> 1/(1 + (0.00043 mM/c)^4.8);

    bind v = voltage;
    bind cai = internal concentration "ca";

    initial state = zInf(v, cai);
    evolve state' = (zInf(v, cai) - state)/zTau;
    effect current density "k" = gbar*state*(v - ek);
}
