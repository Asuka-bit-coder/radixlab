package com.radixlab.backend.controller;

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
import com.radixlab.backend.dto.BooleanTruthTableRowResponse;
import com.radixlab.backend.exception.BooleanLimitException;
import com.radixlab.backend.exception.BooleanParseException;
import com.radixlab.backend.exception.InvalidBooleanRequestException;
import com.radixlab.backend.service.BooleanService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;
import java.util.Map;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(BooleanController.class)
class BooleanControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockitoBean
    private BooleanService booleanService;

    private final ObjectMapper objectMapper = new ObjectMapper();

    private JsonNode ast(String type) {
        var node = objectMapper.createObjectNode();
        node.put("type", type);
        return node;
    }

    @Test
    void returnsEvaluation() throws Exception {
        when(booleanService.evaluate(any(BooleanEvaluateRequest.class)))
                .thenReturn(new BooleanEvaluateResponse(
                        "A & B", "(A & B)", List.of("A", "B"),
                        Map.of("A", true, "B", true), true, ast("and")));

        mockMvc.perform(post("/api/v1/boolean/evaluate")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"expression":"A & B",
                                 "assignment":{"A":true,"B":true}}
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.result").value(true))
                .andExpect(jsonPath("$.ast.type").value("and"))
                .andExpect(jsonPath("$.normalized").value("(A & B)"));
    }

    @Test
    void returnsTruthTable() throws Exception {
        when(booleanService.truthTable(any(BooleanExpressionRequest.class)))
                .thenReturn(new BooleanTruthTableResponse(
                        "A", "A", List.of("A"), 2,
                        List.of(
                                new BooleanTruthTableRowResponse(
                                        0, Map.of("A", false), false),
                                new BooleanTruthTableRowResponse(
                                        1, Map.of("A", true), true)
                        ),
                        ast("variable")));

        mockMvc.perform(post("/api/v1/boolean/truth-table")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"expression\":\"A\"}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.rowCount").value(2))
                .andExpect(jsonPath("$.rows[1].result").value(true));
    }

    @Test
    void returnsCanonicalForms() throws Exception {
        when(booleanService.canonicalForms(any(BooleanExpressionRequest.class)))
                .thenReturn(new BooleanCanonicalResponse(
                        "A ^ B", "(A ^ B)", List.of("A", "B"), 4,
                        List.of(1, 2), List.of(0, 3),
                        "(!A & B) | (A & !B)",
                        "(A | B) & (!A | !B)",
                        ast("xor")));

        mockMvc.perform(post("/api/v1/boolean/canonical")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"expression\":\"A ^ B\"}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.minterms[0]").value(1))
                .andExpect(jsonPath("$.maxterms[1]").value(3))
                .andExpect(jsonPath("$.sop").value("(!A & B) | (A & !B)"));
    }

    @Test
    void returnsEquivalenceCounterexample() throws Exception {
        when(booleanService.equivalent(any(BooleanEquivalentRequest.class)))
                .thenReturn(new BooleanEquivalentResponse(
                        "A & B", "A | B", "(A & B)", "(A | B)",
                        List.of("A", "B"), false, ast("and"), ast("or"),
                        new BooleanCounterexampleResponse(
                                Map.of("A", false, "B", true), false, true)));

        mockMvc.perform(post("/api/v1/boolean/equivalent")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"leftExpression":"A & B",
                                 "rightExpression":"A | B"}
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.equivalent").value(false))
                .andExpect(jsonPath("$.counterexample.assignment.A").value(false))
                .andExpect(jsonPath("$.counterexample.left").value(false))
                .andExpect(jsonPath("$.counterexample.right").value(true));
    }

    @Test
    void rejectsBlankExpression() throws Exception {
        mockMvc.perform(post("/api/v1/boolean/truth-table")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"expression\":\" \"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value(
                        "expression must not be blank"));
    }

    @Test
    void mapsParsePosition() throws Exception {
        when(booleanService.truthTable(any(BooleanExpressionRequest.class)))
                .thenThrow(new BooleanParseException(
                        "Expected a variable", 4, 5));

        mockMvc.perform(post("/api/v1/boolean/truth-table")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"expression\":\"A & | B\"}"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("parse_error"))
                .andExpect(jsonPath("$.position").value(4))
                .andExpect(jsonPath("$.column").value(5));
    }

    @Test
    void mapsLimitAndInvalidAssignmentErrors() throws Exception {
        when(booleanService.truthTable(any(BooleanExpressionRequest.class)))
                .thenThrow(new BooleanLimitException("too many variables"));
        when(booleanService.evaluate(any(BooleanEvaluateRequest.class)))
                .thenThrow(new InvalidBooleanRequestException(
                        "Assignment names must be uppercase"));

        mockMvc.perform(post("/api/v1/boolean/truth-table")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{\"expression\":\"A\"}"))
                .andExpect(status().isUnprocessableEntity())
                .andExpect(jsonPath("$.code").value("limit"));

        mockMvc.perform(post("/api/v1/boolean/evaluate")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"expression":"A","assignment":{"a":true}}
                                """))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.code").value("invalid_request"));
    }
}
