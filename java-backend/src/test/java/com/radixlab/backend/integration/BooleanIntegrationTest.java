package com.radixlab.backend.integration;

import com.radixlab.backend.dto.BooleanCanonicalResponse;
import com.radixlab.backend.dto.BooleanEquivalentRequest;
import com.radixlab.backend.dto.BooleanEquivalentResponse;
import com.radixlab.backend.dto.BooleanEvaluateRequest;
import com.radixlab.backend.dto.BooleanEvaluateResponse;
import com.radixlab.backend.dto.BooleanExpressionRequest;
import com.radixlab.backend.dto.BooleanTruthTableResponse;
import org.junit.jupiter.api.Tag;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;

import static org.assertj.core.api.Assertions.assertThat;

@Tag("integration")
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class BooleanIntegrationTest {

    private static final Path CLI_PATH = Path.of(System.getenv()
            .getOrDefault("RADIXLAB_CLI_PATH", "../cpp-core/build/radixlab-cli"))
            .toAbsolutePath()
            .normalize();

    @Autowired
    private TestRestTemplate restTemplate;

    @DynamicPropertySource
    static void engineProperties(DynamicPropertyRegistry registry) {
        if (!Files.isExecutable(CLI_PATH)) {
            throw new IllegalStateException(
                    "Integration test requires the C++ CLI at " + CLI_PATH
                            + ". Build cpp-core or set RADIXLAB_CLI_PATH.");
        }
        registry.add("radixlab.engine.cli-path", CLI_PATH::toString);
        registry.add("radixlab.engine.timeout", () -> "5s");
    }

    @Test
    void servesBooleanLabPage() {
        ResponseEntity<String> response = restTemplate.getForEntity(
                "/boolean.html", String.class);

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody())
                .contains("Boolean Lab")
                .contains("AST Explorer")
                .contains("boolean.js");
    }

    @Test
    void evaluatesThroughRealEngine() {
        ResponseEntity<BooleanEvaluateResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/boolean/evaluate",
                        new BooleanEvaluateRequest(
                                "A & (!B | C)",
                                Map.of("A", true, "B", false, "C", false)
                        ),
                        BooleanEvaluateResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().result()).isTrue();
        assertThat(response.getBody().normalized())
                .isEqualTo("(A & (!B | C))");
        assertThat(response.getBody().ast().path("type").asText())
                .isEqualTo("and");
    }

    @Test
    void generatesTruthTableThroughRealEngine() {
        ResponseEntity<BooleanTruthTableResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/boolean/truth-table",
                        new BooleanExpressionRequest("A & B"),
                        BooleanTruthTableResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().variables()).containsExactly("A", "B");
        assertThat(response.getBody().rowCount()).isEqualTo(4);
        assertThat(response.getBody().rows().get(3).result()).isTrue();
        assertThat(response.getBody().rows().get(0).result()).isFalse();
    }

    @Test
    void generatesCanonicalFormsThroughRealEngine() {
        ResponseEntity<BooleanCanonicalResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/boolean/canonical",
                        new BooleanExpressionRequest("A ^ B"),
                        BooleanCanonicalResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().minterms()).containsExactly(1, 2);
        assertThat(response.getBody().maxterms()).containsExactly(0, 3);
        assertThat(response.getBody().sop())
                .isEqualTo("(!A & B) | (A & !B)");
        assertThat(response.getBody().pos())
                .isEqualTo("(A | B) & (!A | !B)");
    }

    @Test
    void verifiesDeMorganAndReturnsCounterexample() {
        ResponseEntity<BooleanEquivalentResponse> equivalent =
                restTemplate.postForEntity(
                        "/api/v1/boolean/equivalent",
                        new BooleanEquivalentRequest(
                                "!(A & B)", "!A | !B"),
                        BooleanEquivalentResponse.class
                );

        ResponseEntity<BooleanEquivalentResponse> different =
                restTemplate.postForEntity(
                        "/api/v1/boolean/equivalent",
                        new BooleanEquivalentRequest(
                                "A & B", "A | B"),
                        BooleanEquivalentResponse.class
                );

        assertThat(equivalent.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(equivalent.getBody()).isNotNull();
        assertThat(equivalent.getBody().equivalent()).isTrue();
        assertThat(equivalent.getBody().counterexample()).isNull();

        assertThat(different.getBody()).isNotNull();
        assertThat(different.getBody().equivalent()).isFalse();
        assertThat(different.getBody().counterexample()).isNotNull();
        assertThat(different.getBody().counterexample().assignment())
                .containsEntry("A", false)
                .containsEntry("B", true);
        assertThat(different.getBody().counterexample().left()).isFalse();
        assertThat(different.getBody().counterexample().right()).isTrue();
    }

    @Test
    void reportsParsePositionAndLimit() {
        ResponseEntity<String> parseError = restTemplate.postForEntity(
                "/api/v1/boolean/truth-table",
                new BooleanExpressionRequest("A & | B"),
                String.class
        );
        ResponseEntity<String> limitError = restTemplate.postForEntity(
                "/api/v1/boolean/truth-table",
                new BooleanExpressionRequest("A | B | C | D | E | F | G"),
                String.class
        );

        assertThat(parseError.getStatusCode()).isEqualTo(HttpStatus.BAD_REQUEST);
        assertThat(parseError.getBody())
                .contains("\"code\":\"parse_error\"")
                .contains("\"position\":4")
                .contains("\"column\":5")
                .doesNotContain(CLI_PATH.toString());

        assertThat(limitError.getStatusCode())
                .isEqualTo(HttpStatus.UNPROCESSABLE_ENTITY);
        assertThat(limitError.getBody()).contains("6 distinct variables");
    }
}
