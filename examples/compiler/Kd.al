mechanism density "Kd" {
    parameter gbar = 1e-5 [S/cm^2];
    parameter ek = -77 [mV];
    bind v = membrane_potential;

    record state_rec {
        m: real,
        h: real,
    };
    state s: state_rec;

    function mInf(v: voltage): real {
        1 - 1/(1 + exp((v + 43 [mV])/8 [mV]))
    };

    function hInf(v: voltage): real {
        1/(1 + exp((v + 67 [mV])/7.3 [mV]));
    }

    function state0(v: voltage): state_rec {
        state_rec {
            m = mInf(v);
            h = hInf(v);
        };
    };

    function rate(s: state_rec, v: voltage): state_rec' {
        state_rec'{
            m' = (s.m - mInf(v))/1 [ms];
            h' = (s.h - hInf(v))/1500 [ms];
        };
    }

    function curr(s: state_rec, v_minus_ek: voltage): current/area {
        gbar*s.m*s.h*v_minus_ek;
    }

    initial s = state0(v);
    evolve s' = rate(s, v);
    effect current_density("k") = curr(s, v - ek);
    
    export gbar; 
}
