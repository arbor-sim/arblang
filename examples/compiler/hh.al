mechanism density "hh" {
    # parameters
    parameter gnabar = 0.12   [S/cm^2];
    parameter ena    = 50     [mV];
    parameter gkbar  = 0.036  [S/cm^2];
    parameter ek     = -77    [mV];
    parameter gl     = 0.0003 [S/cm^2];
    parameter el     = -54.3  [mV];

    # bindings
    bind v = membrane_potential;
    bind temp = temperature;

    # state
    record state_rec {
        m: real,
        h: real,
        n: real,
    };
    state s: state_rec;

    # helper functions
    function vtrap(x:real, y:real): real {
        y*exprelr(x/y);
    };

    function m_alpha(v:voltage): real {
        0.1*vtrap(-(v + 40[mV])/1[mV], 10);
    };

    function h_alpha(v:voltage): real {
        0.07*exp(-(v + 65[mV])/20[mV]);
    };

    function n_alpha(v:voltage): real {
        0.01*vtrap(-(v + 55[mV])/1[mV], 10);
    };

    function m_beta(v:voltage):real {
        4.0*exp(-(v + 65[mV])/18[mV]);
    };

    function h_beta(v:voltage): real {
        1.0/(exp(-(v + 35[mV])/10[mV]) + 1);
    };

    function n_beta(v:voltage): real {
        0.125*exp(-(v + 65[mV])/80[mV]);
    };

    function init_state(v: voltage): state_rec {
        state_rec{
            m = (m_alpha(v))/(m_alpha(v) + m_beta(v));
            h = (h_alpha(v))/(h_alpha(v) + h_beta(v));
            n = (n_alpha(v))/(n_alpha(v) + n_beta(v));
        };
    }

    function evolve_state(s: state_rec, v: voltage): state_rec' {
        let q10:real = 3^((temp - 6.3[K])/10.0[K]);
        state_rec'{
            m' = (m_alpha(v) - s.m*(m_alpha(v)+m_beta(v)))*q10/1[s];
            h' = (h_alpha(v) - s.h*(h_alpha(v)+h_beta(v)))*q10/1[s];
            n' = (n_alpha(v) - s.n*(n_alpha(v)+n_beta(v)))*q10/1[s];
        };
    }

    initial s = init_state(v);
    evolve s' = evolve_state(s, v);
    effect current_density("k")  = gkbar * s.n^4 * (v-ek);
    effect current_density("na") = gnabar* s.m^3 * s.h * (v-ena);
    effect current_density       = gl*(v-el);

    export gnabar;
    export ena;
    export gkbar;
    export ek;
    export gl;
    export el;
}
