package com.radixlab.backend.dto;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanCanonicalResponse(
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
