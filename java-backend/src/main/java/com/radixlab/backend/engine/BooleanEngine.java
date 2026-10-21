package com.radixlab.backend.engine;

import java.util.Map;

public interface BooleanEngine {

    BooleanEvaluationResult evaluate(
            String expression,
            Map<String, Boolean> assignment
    );

    BooleanTruthTableResult truthTable(String expression);

    BooleanCanonicalResult canonicalForms(String expression);

    BooleanEquivalenceResult equivalent(
            String leftExpression,
            String rightExpression
    );
}
