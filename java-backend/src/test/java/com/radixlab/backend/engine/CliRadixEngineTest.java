package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.InvalidNumberException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class CliRadixEngineTest {

    private EngineProperties properties;

    @BeforeEach
    void setUp() {
        properties = new EngineProperties();
        properties.setCliPath("/opt/radixlab/radixlab-cli");
        properties.setTimeout(Duration.ofSeconds(3));
        properties.setMaxConcurrent(2);
    }

    @Test
    void invokesCliWithoutShellAndParsesResult() {
        AtomicReference<List<String>> captured = new AtomicReference<>();
        ProcessExecutor executor = (command, timeout) -> {
            captured.set(command);
            return new ProcessOutput(
                    0,
                    "{\"result\":\"2D.6\",\"exact\":true,"
                            + "\"truncated\":false,\"repeating\":false}",
                    ""
            );
        };
        CliRadixEngine engine = new CliRadixEngine(
                executor, new ObjectMapper(), properties);

        EngineResult result = engine.convert(
                new ConversionCommand("101101.011", 2, 16, 256));

        assertThat(result.result()).isEqualTo("2D.6");
        assertThat(result.exact()).isTrue();
        assertThat(captured.get()).containsExactly(
                "/opt/radixlab/radixlab-cli",
                "--input", "101101.011",
                "--from", "2",
                "--to", "16",
                "--max-fraction-digits", "256",
                "--json"
        );
    }

    @Test
    void mapsCliValidationFailureToInvalidNumber() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                2,
                "",
                "{\"error\":{\"code\":\"invalid_number\","
                        + "\"message\":\"Digit out of range for radix\"}}"
        );
        CliRadixEngine engine = new CliRadixEngine(
                executor, new ObjectMapper(), properties);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("102", 2, 10, 256)))
                .isInstanceOf(InvalidNumberException.class)
                .hasMessage("Digit out of range for radix");
    }

    @Test
    void mapsAbnormalExitToEngineFailure() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(1, "", "segmentation-style internal failure");
        CliRadixEngine engine = new CliRadixEngine(
                executor, new ObjectMapper(), properties);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Conversion engine failed");
    }

    @Test
    void rejectsMalformedEngineJson() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(0, "not-json", "");
        CliRadixEngine engine = new CliRadixEngine(
                executor, new ObjectMapper(), properties);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Conversion engine returned an invalid response");
    }

    @Test
    void mapsProcessTimeout() {
        ProcessExecutor executor = (command, timeout) -> {
            throw new ProcessTimeoutException("timed out", null);
        };
        CliRadixEngine engine = new CliRadixEngine(
                executor, new ObjectMapper(), properties);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineTimeoutException.class)
                .hasMessage("Conversion engine timed out");
    }
}
