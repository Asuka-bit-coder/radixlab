package com.radixlab.backend.engine;

import java.util.Map;

public record BooleanCounterexample(
        Map<String, Boolean> assignment,
        boolean left,
        boolean right
) {
}
