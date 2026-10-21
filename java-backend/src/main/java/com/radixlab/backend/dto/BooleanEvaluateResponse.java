package com.radixlab.backend.dto;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;
import java.util.Map;

public record BooleanEvaluateResponse(
        String expression,
        String normalized,
        List<String> variables,
        Map<String, Boolean> assignment,
        boolean result,
        JsonNode ast
) {
}
