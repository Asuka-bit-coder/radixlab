package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.BooleanLimitException;
import com.radixlab.backend.exception.BooleanParseException;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class CliBooleanEngineTest {

    private EngineProperties properties;

    @BeforeEach
    void setUp() {
        properties = new EngineProperties();
        properties.setCliPath("/opt/radixlab/radixlab-cli");
        properties.setTimeout(Duration.ofSeconds(3));
        properties.setMaxConcurrent(2);
    }

    private CliBooleanEngine engine(ProcessExecutor executor) {
        return new CliBooleanEngine(
                new CliEngineClient(executor, properties),
                new ObjectMapper()
        );
    }

    @Test
    void evaluatesAndBuildsCommandWithAssignment() {
        AtomicReference<List<String>> captured = new AtomicReference<>();
        ProcessExecutor executor = (command, timeout) -> {
            captured.set(command);
            return new ProcessOutput(
                    0,
                    """
                    {"expression":"A & B","normalized":"(A & B)",
                     "variables":["A","B"],
                     "assignment":{"A":true,"B":false},
                     "result":false,
                     "ast":{"type":"and",
                       "left":{"type":"variable","name":"A"},
                       "right":{"type":"variable","name":"B"}}}
                    """,
                    ""
            );
        };

        BooleanEvaluationResult result = engine(executor).evaluate(
                "A & B",
                Map.of("B", false, "A", true)
        );

        assertThat(result.result()).isFalse();
        assertThat(result.ast().path("type").asText()).isEqualTo("and");
        assertThat(result.assignment()).containsEntry("A", true);
        assertThat(captured.get()).containsExactly(
                "/opt/radixlab/radixlab-cli",
                "boolean", "evaluate",
                "--expression", "A & B",
                "--assignment", "A=1,B=0",
                "--json"
        );
    }

    @Test
    void parsesTruthTableRows() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                0,
                """
                {"expression":"A","normalized":"A","variables":["A"],
                 "rowCount":2,
                 "rows":[
                   {"index":0,"assignment":{"A":false},"result":false},
                   {"index":1,"assignment":{"A":true},"result":true}
                 ],
                 "ast":{"type":"variable","name":"A"}}
                """,
                ""
        );

        BooleanTruthTableResult result = engine(executor).truthTable("A");

        assertThat(result.rowCount()).isEqualTo(2);
        assertThat(result.rows()).hasSize(2);
        assertThat(result.rows().get(1).result()).isTrue();
    }

    @Test
    void parsesEquivalenceCounterexample() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                0,
                """
                {"leftExpression":"A & B","rightExpression":"A | B",
                 "leftNormalized":"(A & B)","rightNormalized":"(A | B)",
                 "variables":["A","B"],"equivalent":false,
                 "leftAst":{"type":"and"},
                 "rightAst":{"type":"or"},
                 "counterexample":{"assignment":{"A":false,"B":true},
                   "left":false,"right":true}}
                """,
                ""
        );

        BooleanEquivalenceResult result = engine(executor).equivalent(
                "A & B", "A | B");

        assertThat(result.equivalent()).isFalse();
        assertThat(result.counterexample()).isNotNull();
        assertThat(result.counterexample().assignment())
                .containsEntry("B", true);
        assertThat(result.counterexample().left()).isFalse();
    }

    @Test
    void mapsParseErrorPosition() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                2,
                "",
                """
                {"error":{"code":"parse_error",
                 "message":"Expected a variable, 0, 1, '!', or '('",
                 "position":4,"column":5}}
                """.replace("'", "\\u0027")
        );

        assertThatThrownBy(() -> engine(executor).truthTable("A & | B"))
                .isInstanceOf(BooleanParseException.class)
                .satisfies(error -> {
                    BooleanParseException parse = (BooleanParseException) error;
                    assertThat(parse.position()).isEqualTo(4);
                    assertThat(parse.column()).isEqualTo(5);
                });
    }

    @Test
    void mapsVariableLimit() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                3,
                "",
                "{\"error\":{\"code\":\"limit\","
                        + "\"message\":\"Truth tables are limited to 6 distinct variables\"}}"
        );

        assertThatThrownBy(() -> engine(executor).truthTable(
                "A | B | C | D | E | F | G"))
                .isInstanceOf(BooleanLimitException.class);
    }

    @Test
    void mapsProcessTimeout() {
        ProcessExecutor executor = (command, timeout) -> {
            throw new ProcessTimeoutException("timed out", null);
        };

        assertThatThrownBy(() -> engine(executor).canonicalForms("A"))
                .isInstanceOf(EngineTimeoutException.class)
                .hasMessage("Conversion engine timed out");
    }

    @Test
    void rejectsMalformedBooleanJson() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(0, "{\"normalized\":\"A\"}", "");

        assertThatThrownBy(() -> engine(executor).truthTable("A"))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Boolean engine returned an invalid response");
    }
}
