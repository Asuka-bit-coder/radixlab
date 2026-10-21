package com.radixlab.backend.dto;

import java.util.Map;

public record BooleanCounterexampleResponse(
        Map<String, Boolean> assignment,
        boolean left,
        boolean right
) {
}
