mechanism point "expsyn" {
    # parameters
    parameter tau = 2.0 [ms];
    parameter e   = 0   [mV];

    # states
    state g: conductance;

    # bindings
    bind v = membrane_potential;

    # initial
    initial g = 0 [S];

    # effects
    effect current = g*(v-e);

    # evolutions
    evolve g' = -g/tau;

    # on_events
    on_event(w:conductance) g = g + w;

    # parameter exports
    export tau;
    export e;
}
