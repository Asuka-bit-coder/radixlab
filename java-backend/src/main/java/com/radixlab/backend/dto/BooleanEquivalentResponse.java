package com.radixlab.backend.dto;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanEquivalentResponse(
        String leftExpression,
        String rightExpression,
        String leftNormalized,
        String rightNormalized,
        List<String> variables,
        boolean equivalent,
        JsonNode leftAst,
        JsonNode rightAst,
        BooleanCounterexampleResponse counterexample
) {
}
