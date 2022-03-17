mechanism density "pas" {
    # parameters
    parameter g = 0.001 [S/cm^2];
    parameter e = -70   [mV];

    # bindings
    bind v = membrane_potential;

    # effects
    effect current_density = g*(v-e);

    # parameter exports
    export g;
    export e;
}
