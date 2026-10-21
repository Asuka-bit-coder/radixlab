package com.radixlab.backend.integration;

import com.radixlab.backend.dto.ConvertRequest;
import com.radixlab.backend.dto.ConvertResponse;
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

import static org.assertj.core.api.Assertions.assertThat;

@Tag("integration")
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class ConversionIntegrationTest {

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
    void convertsThroughJavaAndTheRealCppCli() {
        ResponseEntity<ConvertResponse> response = restTemplate.postForEntity(
                "/api/v1/convert",
                new ConvertRequest("101101.011", 2, 16, 256),
                ConvertResponse.class
        );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().result()).isEqualTo("2D.6");
        assertThat(response.getBody().exact()).isTrue();
        assertThat(response.getBody().truncated()).isFalse();
    }

    @Test
    void reportsCycleAndTruncationFromRealEngine() {
        ResponseEntity<ConvertResponse> cycle = restTemplate.postForEntity(
                "/api/v1/convert",
                new ConvertRequest("0.1", 10, 2, 256),
                ConvertResponse.class
        );
        ResponseEntity<ConvertResponse> truncated = restTemplate.postForEntity(
                "/api/v1/convert",
                new ConvertRequest("0.1", 10, 2, 2),
                ConvertResponse.class
        );

        assertThat(cycle.getBody()).isNotNull();
        assertThat(cycle.getBody().result()).isEqualTo("0.0(0011)");
        assertThat(cycle.getBody().repeating()).isTrue();
        assertThat(truncated.getBody()).isNotNull();
        assertThat(truncated.getBody().result()).isEqualTo("0.00");
        assertThat(truncated.getBody().truncated()).isTrue();
        assertThat(truncated.getBody().exact()).isFalse();
    }

    @Test
    void servesTheWebInterface() {
        ResponseEntity<String> response = restTemplate.getForEntity(
                "/", String.class);

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody())
                .contains("RadixLab")
                .contains("/api/v1/convert");
    }

    @Test
    void propagatesInvalidDigitAsClientError() {
        ResponseEntity<String> response = restTemplate.postForEntity(
                "/api/v1/convert",
                new ConvertRequest("102", 2, 10, 256),
                String.class
        );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.BAD_REQUEST);
        assertThat(response.getBody())
                .contains("Digit out of range for radix")
                .doesNotContain(CLI_PATH.toString());
    }
}
