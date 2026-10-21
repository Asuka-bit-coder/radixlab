package com.radixlab.backend.dto;

import java.util.Map;

public record BooleanTruthTableRowResponse(
        int index,
        Map<String, Boolean> assignment,
        boolean result
) {
}
