package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;
import java.util.Map;

public record BooleanEvaluationResult(
        String expression,
        String normalized,
        List<String> variables,
        Map<String, Boolean> assignment,
        boolean result,
        JsonNode ast
) {
}
