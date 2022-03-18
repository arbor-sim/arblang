mechanism point "exp2syn" {
    # parameters
    parameter tau1 = 0.5 [ms];
    parameter tau2 = 2   [ms];
    parameter e    = 0   [mV];

    parameter tp = (tau1*tau2)/(tau2 - tau1) * log(tau2/tau1);
    parameter factor =  1 / (-exp(-tp/tau1) + exp(-tp/tau2));

    # states
    state s: {
        A:real,
        B:real,
    };;

    # bindings
    bind v = membrane_potential;

    # initial
    initial s = {
        A = 0;
        B = 0;
    };

    # effects
    effect current = (s.B-s.A)*1[S] *(v-e);

    # evolutions
    evolve s' = {
        A' = -s.A/tau1;
        B' = -s.B/tau2;
    };

    # on_events
    on_event(w:real) s =
        {
            A = s.A + w*factor;
            B = s.B + w*factor;
        };

    # parameter exports
    export tau1;
    export tau2;
    export e;
}
