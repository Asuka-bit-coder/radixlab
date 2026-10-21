package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.exception.BooleanLimitException;
import com.radixlab.backend.exception.BooleanParseException;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.InvalidBooleanRequestException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

@Component
public class CliBooleanEngine implements BooleanEngine {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(CliBooleanEngine.class);

    private final CliEngineClient client;
    private final ObjectMapper objectMapper;

    public CliBooleanEngine(
            CliEngineClient client,
            ObjectMapper objectMapper
    ) {
        this.client = client;
        this.objectMapper = objectMapper;
    }

    @Override
    public BooleanEvaluationResult evaluate(
            String expression,
            Map<String, Boolean> assignment
    ) {
        List<String> arguments = new ArrayList<>(List.of(
                "boolean", "evaluate",
                "--expression", expression
        ));

        if (assignment != null && !assignment.isEmpty()) {
            arguments.add("--assignment");
            arguments.add(formatAssignment(assignment));
        }
        arguments.add("--json");

        return interpretEvaluation(invoke(arguments));
    }

    @Override
    public BooleanTruthTableResult truthTable(String expression) {
        return interpretTruthTable(invoke(List.of(
                "boolean", "truth-table",
                "--expression", expression,
                "--json"
        )));
    }

    @Override
    public BooleanCanonicalResult canonicalForms(String expression) {
        return interpretCanonical(invoke(List.of(
                "boolean", "canonical-forms",
                "--expression", expression,
                "--json"
        )));
    }

    @Override
    public BooleanEquivalenceResult equivalent(
            String leftExpression,
            String rightExpression
    ) {
        return interpretEquivalence(invoke(List.of(
                "boolean", "equivalent",
                "--left", leftExpression,
                "--right", rightExpression,
                "--json"
        )));
    }

    private ProcessOutput invoke(List<String> arguments) {
        return client.execute(arguments);
    }

    private BooleanEvaluationResult interpretEvaluation(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            return new BooleanEvaluationResult(
                    CliJsonSupport.requiredText(root, "expression"),
                    CliJsonSupport.requiredText(root, "normalized"),
                    textList(root, "variables"),
                    assignment(root.path("assignment")),
                    CliJsonSupport.requiredBoolean(root, "result"),
                    ast(root, "ast")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private BooleanTruthTableResult interpretTruthTable(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            JsonNode rowsNode = root.path("rows");
            if (!rowsNode.isArray()) {
                throw new IOException("Engine JSON is missing rows array");
            }

            List<BooleanTruthTableRow> rows = new ArrayList<>();
            for (JsonNode row : rowsNode) {
                rows.add(new BooleanTruthTableRow(
                        CliJsonSupport.requiredInt(row, "index"),
                        assignment(row.path("assignment")),
                        CliJsonSupport.requiredBoolean(row, "result")
                ));
            }

            return new BooleanTruthTableResult(
                    CliJsonSupport.requiredText(root, "expression"),
                    CliJsonSupport.requiredText(root, "normalized"),
                    textList(root, "variables"),
                    CliJsonSupport.requiredInt(root, "rowCount"),
                    List.copyOf(rows),
                    ast(root, "ast")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private BooleanCanonicalResult interpretCanonical(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            return new BooleanCanonicalResult(
                    CliJsonSupport.requiredText(root, "expression"),
                    CliJsonSupport.requiredText(root, "normalized"),
                    textList(root, "variables"),
                    CliJsonSupport.requiredInt(root, "rowCount"),
                    intList(root, "minterms"),
                    intList(root, "maxterms"),
                    CliJsonSupport.requiredText(root, "sop"),
                    CliJsonSupport.requiredText(root, "pos"),
                    ast(root, "ast")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private BooleanEquivalenceResult interpretEquivalence(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            JsonNode counterexampleNode = root.path("counterexample");
            BooleanCounterexample counterexample = null;

            if (!counterexampleNode.isNull()
                    && !counterexampleNode.isMissingNode()) {
                if (!counterexampleNode.isObject()) {
                    throw new IOException(
                            "Engine counterexample is not an object"
                    );
                }
                counterexample = new BooleanCounterexample(
                        assignment(counterexampleNode.path("assignment")),
                        CliJsonSupport.requiredBoolean(
                                counterexampleNode, "left"),
                        CliJsonSupport.requiredBoolean(
                                counterexampleNode, "right")
                );
            }

            return new BooleanEquivalenceResult(
                    CliJsonSupport.requiredText(root, "leftExpression"),
                    CliJsonSupport.requiredText(root, "rightExpression"),
                    CliJsonSupport.requiredText(root, "leftNormalized"),
                    CliJsonSupport.requiredText(root, "rightNormalized"),
                    textList(root, "variables"),
                    CliJsonSupport.requiredBoolean(root, "equivalent"),
                    ast(root, "leftAst"),
                    ast(root, "rightAst"),
                    counterexample
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private JsonNode successJson(ProcessOutput output) {
        if (output.exitCode() == 3) {
            throw new BooleanLimitException(
                    CliJsonSupport.engineErrorMessage(
                            output,
                            objectMapper,
                            "The Boolean request exceeds an engine limit"
                    )
            );
        }

        if (output.exitCode() == 2) {
            throw clientError(output);
        }

        if (output.exitCode() != 0) {
            LOGGER.error(
                    "Boolean engine failed with exit code {}: {}",
                    output.exitCode(),
                    CliJsonSupport.diagnostic(output.stderr())
            );
            throw new EngineFailureException(
                    "Boolean engine failed", null);
        }

        try {
            return CliJsonSupport.requireObject(
                    output.stdout(), objectMapper);
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private RuntimeException clientError(ProcessOutput output) {
        String fallback = "The Boolean request is invalid";

        try {
            JsonNode root = objectMapper.readTree(output.stderr());
            if (root != null && root.isObject()) {
                JsonNode error = root.path("error");
                String code = error.path("code").asText("");
                String message = error.path("message").asText(fallback);

                if ("parse_error".equals(code)) {
                    int position = error.path("position").asInt(0);
                    int column = error.path("column").asInt(position + 1);
                    return new BooleanParseException(message, position, column);
                }

                if (!message.isBlank()) {
                    return new InvalidBooleanRequestException(message);
                }
            }
        } catch (IOException ignored) {
            // Use the stable fallback below.
        }

        return new InvalidBooleanRequestException(fallback);
    }

    private List<String> textList(JsonNode root, String field) throws IOException {
        JsonNode node = root.path(field);
        if (!node.isArray()) {
            throw new IOException("Engine JSON is missing array field: " + field);
        }

        List<String> values = new ArrayList<>();
        for (JsonNode item : node) {
            if (!item.isTextual()) {
                throw new IOException("Engine array contains a non-string value");
            }
            values.add(item.textValue());
        }
        return List.copyOf(values);
    }

    private List<Integer> intList(JsonNode root, String field) throws IOException {
        JsonNode node = root.path(field);
        if (!node.isArray()) {
            throw new IOException("Engine JSON is missing array field: " + field);
        }

        List<Integer> values = new ArrayList<>();
        for (JsonNode item : node) {
            if (!item.isIntegralNumber() || !item.canConvertToInt()) {
                throw new IOException("Engine array contains a non-integer value");
            }
            values.add(item.intValue());
        }
        return List.copyOf(values);
    }

    private Map<String, Boolean> assignment(JsonNode node) throws IOException {
        if (!node.isObject()) {
            throw new IOException("Engine JSON is missing assignment object");
        }

        Map<String, Boolean> values = new TreeMap<>();
        var fields = node.fields();
        while (fields.hasNext()) {
            Map.Entry<String, JsonNode> entry = fields.next();
            if (entry.getKey().length() != 1
                    || entry.getKey().charAt(0) < 'A'
                    || entry.getKey().charAt(0) > 'Z'
                    || !entry.getValue().isBoolean()) {
                throw new IOException(
                        "Engine assignment contains an invalid entry"
                );
            }
            values.put(entry.getKey(), entry.getValue().booleanValue());
        }

        return Collections.unmodifiableMap(new LinkedHashMap<>(values));
    }

    private JsonNode ast(JsonNode root, String field) throws IOException {
        JsonNode node = root.path(field);
        if (!node.isObject()) {
            throw new IOException("Engine JSON is missing AST object");
        }
        return node.deepCopy();
    }

    private String formatAssignment(Map<String, Boolean> assignment) {
        StringBuilder formatted = new StringBuilder();
        for (Map.Entry<String, Boolean> entry
                : new TreeMap<>(assignment).entrySet()) {
            if (formatted.length() > 0) {
                formatted.append(',');
            }
            formatted.append(entry.getKey())
                    .append('=')
                    .append(Boolean.TRUE.equals(entry.getValue()) ? '1' : '0');
        }
        return formatted.toString();
    }

    private EngineFailureException invalidResponse(
            ProcessOutput output,
            IOException cause
    ) {
        LOGGER.error(
                "Boolean engine returned an invalid response: {}",
                CliJsonSupport.diagnostic(output.stdout()),
                cause
        );
        return new EngineFailureException(
                "Boolean engine returned an invalid response",
                cause
        );
    }
}
