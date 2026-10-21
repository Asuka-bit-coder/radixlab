package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanCanonicalResult(
        String expression,
        String normalized,
        List<String> variables,
        int rowCount,
        List<Integer> minterms,
        List<Integer> maxterms,
        String sop,
        String pos,
        JsonNode ast
) {
}
