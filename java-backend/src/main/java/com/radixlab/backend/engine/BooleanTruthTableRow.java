package com.radixlab.backend.engine;

import java.util.Map;

public record BooleanTruthTableRow(
        int index,
        Map<String, Boolean> assignment,
        boolean result
) {
}
