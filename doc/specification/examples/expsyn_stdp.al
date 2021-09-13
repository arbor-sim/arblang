# Based on Arbor default/expsyn_stdp.mod

interface discrete "expsyn_stdp" {
    # A scaling factor for incoming (pre-synaptic) events is required, as the
    # weight of an event is dimensionelss.

    export parameter A     =  1 μS;    # pre-synaptic event contribution factor
    export parameter Apre  =  0.01 μS; # pre-synaptic event plasticity contribution
    export parameter Apost = -0.01 μS; # post-synaptic event plasticity contribution

    export parameter τ      = 2 ms; # synaptic time constant
    export parameter τpre  = 10 ms; # pre-synaptic plasticity contribution time constant
    export parameter τpost = 10 ms; # post-synaptic plasticity contribution time constant

    export parameter gmax  = 10 μS; # maximum synaptic conductance
    export parameter e = 0 mV;      # reversal potential

    initial state = {
        g = 0 μS;
        apre = 0 μS;
        apost = 0 μS;
        w_plastic = 0 μS;
    };

    # Expression below could have been written without the qualifying 'S.'
    # by using a 'with S;' or 'with state;' instead.

    bind S = state;

    evolve state' = {
        g' = -S.g/τ;
        apre' = -S.apre/τpre;
        apost'= -S.apost/τpost;
    };

    bind v = membrane potential;
    effect current = S.g*(v - e);

    on w = event; state = {
        # With proposed clamp syntax, this could be:
        # g = (S.g + S.w_plastic + w·A) ↓ [0 S, gmax];

        g = let g = S.g + S.w_plastic + w·A;
            | g < 0 S   → 0 S
            | g > gmax  → gmax
            | otherwise → g;

        apre = S.apre + Apre;
        w_plastic = S.w_plastic + S.apost;
    };

    on δt = post; state = {
        apost = S.apost + Apost;
        w_plastic = S.w_plastic + S.apre;
    };

    # The 'δt' above is ignored; it could be incorporated for an update that accounts for the
    # delay between the post-synaptic event and its triggering of the `on` clause. e.g.
    #
    # on δt = post; state = {
    #     apost = S.apost + Apost·exp(-δt/τpost);
    #     w_plastic = S.w_plastic + S.apre·exp(δt/τpre);
    # };
}
