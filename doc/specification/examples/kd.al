# Adapted from Allen Institute Kd.mod, in turn based on Kd model of Foust et al. 2011.
#
# Three versions, one which takes the reversal potential as a parameter, and one where it
# is computed dynamically from ion concentrations.

module Kd {
    parameter gbar = 10^-5 S/cm^2;

    def mInf = fn (v: voltage) ->
        1 - 1/(1 + exp((v + 43 mV)/8 mV));

    def hInf = fn (v: voltage) ->
        1/(1 + exp((v + 67 mV)/7.3 mV));

    type state = {
        m: real;
        h: real;
    };

    def state0 = fn (v: voltage) ->
        {
            m = mInf(v);
            h = hInf(v);
        };

    def rate = fn (s: state, v: voltage) ->
        with s; {
            m' = (m - mInf(v))/1 ms;
            h' = (h - hInf(v))/1500 ms;
        };

    def current = fn (s: state, v_minus_ek: voltage) ->
        with s;
        gbar*m*h*v_minus_ek: current/area;
}

interface density "Kd" {
    import Kd
    export density parameter Kd.gbar as gbar
    export parameter ek = -77 mV;

    bind v = membrane potential;

    initial state = Kd.state0(v);
    evolve state' = Kd.rate(state, v);
    effect current density "k" = Kd.current(state, v - ek);
}

interface density "Kd_nernst" {
    import Kd
    export density parameter Kd.gbar as gbar

    bind v = membrane potential;
    bind ek = nernst potential "k";

    initial state = Kd.state0(v);
    evolve state' = Kd.rate(state, v);
    effect current density "k" = Kd.current(state, v - ek);
}

# Kd_nernst is equivalent to:
#
# interface density "Kd_nernst" {
#     import Kd
#     export density parameter Kd.gbar as gbar
#
#     bind v = membrane potential;
#     bind T = temperature;
#     bind ki = internal concentration "k";
#     bind ko = external concentration "k";
#     bind kz = charge "k";
#
#     initial state = Kd.state0(v);
#     evolve state' = Kd.rate(state, v);
#     effect current density "k" =
#         let ek = nernst(kz, T, ki, ko);
#         Kd.current(state, v - ek);
# }

