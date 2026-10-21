package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanTruthTableResult(
        String expression,
        String normalized,
        List<String> variables,
        int rowCount,
        List<BooleanTruthTableRow> rows,
        JsonNode ast
) {
}
