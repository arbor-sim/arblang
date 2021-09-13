# Adapted from Allen Institute NaTs.mod, in turn based on model of Colbert and Pan 2002.

interface density "NaTs" {
    type rate = real/time;

    export density parameter gbar    = 10^-5 S/cm^2;

    export parameter mαF: rate       = 0.182 ms⁻¹;
    export parameter mβF: rate       = 0.124 ms⁻¹;
    export parameter mv½: voltage    = -40 mV;
    export parameter mk: voltage     = 6 mV;

    export parameter hαF: rate       = 0.015 ms⁻¹;
    export parameter hβF: rate       = 0.015 ms⁻¹;
    export parameter hv½: voltage    = -66 mV;
    export parameter hk: voltage     = 6 mV;

    def rates = fn (v: voltage, T: temperature) →
        let qt = 2.3^((T-296.15 K)/10 K);

        let mα = mαF·mk/exprel((mv½-v)/mk);
        let mβ = mβF·mk/exprel((v-mv½)/mk);

        let hα = hαF·hk/exprel((v-hv½)/hk);
        let hβ = hβF·hk/exprel((hv½-v)/hk);

        {
            mi = mα/(mα + mβ);
            mτ = 1/(mα + mβ)/qt;

            hi = hα/(hα + hβ);
            hτ = 1/(hα + hβ)/qt;
        };

    bind T = temperature;
    bind v = membrane potential;
    bind ena = nernst potential "na";

    initial state =
        with rates(v, T); {
            m = mi;
            h = hi;
        };

    evolve state' =
        with state;
        with rates(v, T); {
            m' = (mi - m)/mτ;
            h' = (hi - h)/hτ;
        };

    effect current density "na" =
        with state;
        gbar·m³·h·(v - ena);
}

