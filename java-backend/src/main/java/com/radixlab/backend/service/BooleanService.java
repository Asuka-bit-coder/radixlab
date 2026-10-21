package com.radixlab.backend.service;

import com.radixlab.backend.dto.BooleanCanonicalResponse;
import com.radixlab.backend.dto.BooleanCounterexampleResponse;
import com.radixlab.backend.dto.BooleanEquivalentRequest;
import com.radixlab.backend.dto.BooleanEquivalentResponse;
import com.radixlab.backend.dto.BooleanEvaluateRequest;
import com.radixlab.backend.dto.BooleanEvaluateResponse;
import com.radixlab.backend.dto.BooleanExpressionRequest;
import com.radixlab.backend.dto.BooleanTruthTableResponse;
import com.radixlab.backend.dto.BooleanTruthTableRowResponse;
import com.radixlab.backend.engine.BooleanCanonicalResult;
import com.radixlab.backend.engine.BooleanEngine;
import com.radixlab.backend.engine.BooleanEquivalenceResult;
import com.radixlab.backend.engine.BooleanEvaluationResult;
import com.radixlab.backend.engine.BooleanTruthTableResult;
import com.radixlab.backend.exception.InvalidBooleanRequestException;
import org.springframework.stereotype.Service;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

@Service
public class BooleanService {

    private final BooleanEngine engine;

    public BooleanService(BooleanEngine engine) {
        this.engine = engine;
    }

    public BooleanEvaluateResponse evaluate(BooleanEvaluateRequest request) {
        Map<String, Boolean> assignment = validateAssignment(
                request.assignment());

        BooleanEvaluationResult result = engine.evaluate(
                request.expression(), assignment);

        return new BooleanEvaluateResponse(
                result.expression(),
                result.normalized(),
                result.variables(),
                result.assignment(),
                result.result(),
                result.ast()
        );
    }

    public BooleanTruthTableResponse truthTable(
            BooleanExpressionRequest request
    ) {
        BooleanTruthTableResult result = engine.truthTable(
                request.expression());

        List<BooleanTruthTableRowResponse> rows = result.rows().stream()
                .map(row -> new BooleanTruthTableRowResponse(
                        row.index(),
                        row.assignment(),
                        row.result()
                ))
                .toList();

        return new BooleanTruthTableResponse(
                result.expression(),
                result.normalized(),
                result.variables(),
                result.rowCount(),
                rows,
                result.ast()
        );
    }

    public BooleanCanonicalResponse canonicalForms(
            BooleanExpressionRequest request
    ) {
        BooleanCanonicalResult result = engine.canonicalForms(
                request.expression());

        return new BooleanCanonicalResponse(
                result.expression(),
                result.normalized(),
                result.variables(),
                result.rowCount(),
                result.minterms(),
                result.maxterms(),
                result.sop(),
                result.pos(),
                result.ast()
        );
    }

    public BooleanEquivalentResponse equivalent(
            BooleanEquivalentRequest request
    ) {
        BooleanEquivalenceResult result = engine.equivalent(
                request.leftExpression(),
                request.rightExpression()
        );

        BooleanCounterexampleResponse counterexample = null;
        if (result.counterexample() != null) {
            counterexample = new BooleanCounterexampleResponse(
                    result.counterexample().assignment(),
                    result.counterexample().left(),
                    result.counterexample().right()
            );
        }

        return new BooleanEquivalentResponse(
                result.leftExpression(),
                result.rightExpression(),
                result.leftNormalized(),
                result.rightNormalized(),
                result.variables(),
                result.equivalent(),
                result.leftAst(),
                result.rightAst(),
                counterexample
        );
    }

    private static Map<String, Boolean> validateAssignment(
            Map<String, Boolean> assignment
    ) {
        if (assignment == null) {
            return Map.of();
        }

        Map<String, Boolean> sorted = new TreeMap<>();
        for (Map.Entry<String, Boolean> entry : assignment.entrySet()) {
            String name = entry.getKey();
            if (name == null
                    || name.length() != 1
                    || name.charAt(0) < 'A'
                    || name.charAt(0) > 'Z') {
                throw new InvalidBooleanRequestException(
                        "Assignment names must be single uppercase letters A-Z"
                );
            }

            if (entry.getValue() == null) {
                throw new InvalidBooleanRequestException(
                        "Assignment value for " + name + " must be true or false"
                );
            }

            sorted.put(name, entry.getValue());
        }

        return java.util.Collections.unmodifiableMap(
                new LinkedHashMap<>(sorted)
        );
    }
}
