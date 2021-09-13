# Adapted from Allen Institute Ca_dynamics.mod, in turn based on model of Destexhe et al. 1994.

interface concentration "CaDynamics" {
    export parameter gamma = 0.05;    # Proportion of unbuffered calcium.
    export parameter decay = 80 ms;   # Calcium removal time constant.
    export parameter minCai = 1e-4 mM;
    export parameter depth = 0.1 µm;  # Depth of shell.

    bind flux = molar flux "ca";
    bind cai = internal concentration "ca";
    effect molar flux rate "ca" = -gamma*flux - (cai - minCai)/decay;
}
