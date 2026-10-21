package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;

import java.util.List;

public record BooleanEquivalenceResult(
        String leftExpression,
        String rightExpression,
        String leftNormalized,
        String rightNormalized,
        List<String> variables,
        boolean equivalent,
        JsonNode leftAst,
        JsonNode rightAst,
        BooleanCounterexample counterexample
) {
}
