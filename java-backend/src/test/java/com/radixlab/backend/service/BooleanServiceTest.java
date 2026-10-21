package com.radixlab.backend.service;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.dto.BooleanCanonicalResponse;
import com.radixlab.backend.dto.BooleanCounterexampleResponse;
import com.radixlab.backend.dto.BooleanEquivalentRequest;
import com.radixlab.backend.dto.BooleanEquivalentResponse;
import com.radixlab.backend.dto.BooleanEvaluateRequest;
import com.radixlab.backend.dto.BooleanEvaluateResponse;
import com.radixlab.backend.dto.BooleanExpressionRequest;
import com.radixlab.backend.dto.BooleanTruthTableResponse;
import com.radixlab.backend.engine.BooleanCanonicalResult;
import com.radixlab.backend.engine.BooleanCounterexample;
import com.radixlab.backend.engine.BooleanEngine;
import com.radixlab.backend.engine.BooleanEquivalenceResult;
import com.radixlab.backend.engine.BooleanEvaluationResult;
import com.radixlab.backend.engine.BooleanTruthTableResult;
import com.radixlab.backend.engine.BooleanTruthTableRow;
import com.radixlab.backend.exception.InvalidBooleanRequestException;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import java.util.List;
import java.util.Map;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class BooleanServiceTest {

    @Mock
    private BooleanEngine engine;

    @InjectMocks
    private BooleanService service;

    private final ObjectMapper objectMapper = new ObjectMapper();

    private JsonNode ast(String type) {
        var node = objectMapper.createObjectNode();
        node.put("type", type);
        return node;
    }

    @Test
    void mapsEvaluation() {
        when(engine.evaluate("A & B", Map.of("A", true, "B", false)))
                .thenReturn(new BooleanEvaluationResult(
                        "A & B", "(A & B)", List.of("A", "B"),
                        Map.of("A", true, "B", false), false, ast("and")));

        BooleanEvaluateResponse response = service.evaluate(
                new BooleanEvaluateRequest(
                        "A & B", Map.of("A", true, "B", false)));

        assertThat(response.result()).isFalse();
        assertThat(response.ast().path("type").asText()).isEqualTo("and");
        assertThat(response.variables()).containsExactly("A", "B");
    }

    @Test
    void rejectsInvalidAssignmentKeys() {
        assertThatThrownBy(() -> service.evaluate(
                new BooleanEvaluateRequest("A", Map.of("a", true))))
                .isInstanceOf(InvalidBooleanRequestException.class)
                .hasMessageContaining("uppercase");
    }

    @Test
    void mapsTruthTable() {
        when(engine.truthTable("A")).thenReturn(new BooleanTruthTableResult(
                "A", "A", List.of("A"), 2,
                List.of(
                        new BooleanTruthTableRow(0, Map.of("A", false), false),
                        new BooleanTruthTableRow(1, Map.of("A", true), true)
                ),
                ast("variable")));

        BooleanTruthTableResponse response = service.truthTable(
                new BooleanExpressionRequest("A"));

        assertThat(response.rowCount()).isEqualTo(2);
        assertThat(response.rows()).hasSize(2);
        assertThat(response.rows().get(1).result()).isTrue();
    }

    @Test
    void mapsCanonicalForms() {
        when(engine.canonicalForms("A ^ B")).thenReturn(
                new BooleanCanonicalResult(
                        "A ^ B", "(A ^ B)", List.of("A", "B"), 4,
                        List.of(1, 2), List.of(0, 3),
                        "(!A & B) | (A & !B)",
                        "(A | B) & (!A | !B)",
                        ast("xor")));

        BooleanCanonicalResponse response = service.canonicalForms(
                new BooleanExpressionRequest("A ^ B"));

        assertThat(response.minterms()).containsExactly(1, 2);
        assertThat(response.maxterms()).containsExactly(0, 3);
        assertThat(response.sop()).contains("!A");
    }

    @Test
    void mapsEquivalenceAndCounterexample() {
        when(engine.equivalent("A & B", "A | B")).thenReturn(
                new BooleanEquivalenceResult(
                        "A & B", "A | B", "(A & B)", "(A | B)",
                        List.of("A", "B"), false, ast("and"), ast("or"),
                        new BooleanCounterexample(
                                Map.of("A", false, "B", true), false, true)));

        BooleanEquivalentResponse response = service.equivalent(
                new BooleanEquivalentRequest("A & B", "A | B"));

        assertThat(response.equivalent()).isFalse();
        BooleanCounterexampleResponse counterexample = response.counterexample();
        assertThat(counterexample).isNotNull();
        assertThat(counterexample.assignment()).containsEntry("B", true);
        verify(engine).equivalent("A & B", "A | B");
    }
}
