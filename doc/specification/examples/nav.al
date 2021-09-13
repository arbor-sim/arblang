# Adapted from Allen Institute NaV.mod, based on kinetics
# from Carter et al. 2012 doi:10.1016/j.neuron.2012.08.033 (see Figure 7A).

interface density "NaV" {
    export density parameter g̅ = 0.015 S/cm²;

    export parameter Con  =  0.01 ms⁻¹; # closed C1 → inactivated I1 transition
    export parameter Coff = 40    ms⁻¹; # inactivated I1 → closed C1 transitions
    export parameter Oon  =  8    ms⁻¹; # open O → inactivated I6 transition
    export parameter Ooff =  0.05 ms⁻¹; # inactivated I6 → open O transition
    export parameter α =   400    ms⁻¹; # closed Cx right transitions (activation)
    export parameter β =    12    ms⁻¹; # closed Cx left transitions (deactivation)
    export parameter γ =   250    ms⁻¹; # closed → open transition
    export parameter δ =    60    ms⁻¹; # open → closed transition

    export parameter a = 2.51;          # factor for right Ix transitions
    export parameter b = 5.32;          # inverse factor for left Ix transitions

    export parameter αvdep =  24 mV;    # Vdep of activation
    export parameter βvdep = -24 mV;    # Vdep of deactivation

    # With the proposed range-limited extension, fields below would be defined
    #     C1: [0, 1];
    # etc.
    type state = {
        C1: real;
        C2: real;
        C3: real;
        C4: real;
        C5: real;
        O:  real;
        I1: real;
        I2: real;
        I3: real;
        I4: real;
        I5: real;
        I6: real;
    };

    def kinetics = fn(s: state, v: voltage, Q: real) →
        # scale rates by Q and voltage dependencies
        let Con  = Q·Con;
        let Coff = Q·Coff;
        let Oon  = Q·Oon;
        let Ooff = Q·Ooff;
        let α    = Q·α·exp(v/αvdep);
        let β    = Q·β·exp(v/βvdep);
        let γ    = Q·γ;
        let δ    = Q·δ;

        with s; {
            C1 ⇄ C2  ( 4·α,   β );
            C2 ⇄ C3  ( 3·α, 2·β );
            C3 ⇄ C4  ( 2·α, 3·β );
            C4 ⇄ C5  (   α, 4·β );
            C5 ⇄ O   (   γ,   δ );

            I1 ⇄ I2  ( 4·a·α,   β/b );
            I2 ⇄ I2  ( 3·a·α, 2·β/b );
            I3 ⇄ I4  ( 2·a·α, 3·β/b );
            I4 ⇄ I5  (   a·α, 4·β/b );
            I5 ⇄ I6  (     γ,   δ   );

            C1 ⇄ I1  (     Con, Coff    );
            C2 ⇄ I2  (   a·Con, Coff/b  );
            C3 ⇄ I3  (  a²·Con, Coff/b² );
            C4 ⇄ I4  (  a³·Con, Coff/b³ );
            C5 ⇄ I5  (  a⁴·Con, Coff/b⁴ );
            O  ⇄ I6  (     Oon, Ooff    );
        }: state';

    bind v = membrane potential;
    bind T = temperature;
    bind ena = nernst potential "na";

    def qt(T: temperature) → 2.3^((T - 310.15 K)/10 K);

    initial steady state from state =
        { C1 = 1; C2 = 0; C3 = 0; C4 = 0; C5 = 0; O = 0;
          I1 = 0; I2 = 0; I3 = 0; I4 = 0; I5 = 0; I6 = 0; };

    evolve state' = kinetics(state, v, qt(T));

    effect current density "na" = with state; g̅·O·(v - ena);
}
