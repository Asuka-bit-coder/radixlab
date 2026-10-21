package com.radixlab.backend.dto;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanTruthTableResponse(
        String expression,
        String normalized,
        List<String> variables,
        int rowCount,
        List<BooleanTruthTableRowResponse> rows,
        JsonNode ast
) {
}
