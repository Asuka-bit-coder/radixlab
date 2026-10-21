package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.InvalidNumberException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import com.radixlab.backend.exception.EngineBusyException;

import java.time.Duration;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
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

    private CliRadixEngine engine(ProcessExecutor executor) {
        return new CliRadixEngine(
                new CliEngineClient(executor, properties),
                new ObjectMapper()
        );
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
        CliRadixEngine engine = engine(executor);

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
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("102", 2, 10, 256)))
                .isInstanceOf(InvalidNumberException.class)
                .hasMessage("Digit out of range for radix");
    }

    @Test
    void mapsAbnormalExitToEngineFailure() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(1, "", "segmentation-style internal failure");
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Conversion engine failed");
    }

    @Test
    void rejectsMalformedEngineJson() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(0, "not-json", "");
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Conversion engine returned an invalid response");
    }

    @Test
    void rejectsEmptyEngineOutput() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(0, "", "");
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Conversion engine returned an invalid response");
    }

    @Test
    void usesSafeFallbackForEmptyCliError() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(2, "", "");
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(InvalidNumberException.class)
                .hasMessage("The numeral is invalid for the selected source base");
    }

    @Test
    void mapsProcessTimeout() {
        ProcessExecutor executor = (command, timeout) -> {
            throw new ProcessTimeoutException("timed out", null);
        };
        CliRadixEngine engine = engine(executor);

        assertThatThrownBy(() -> engine.convert(
                new ConversionCommand("10", 2, 10, 256)))
                .isInstanceOf(EngineTimeoutException.class)
                .hasMessage("Conversion engine timed out");
    }

    @Test
    void rejectsWorkWhenAllProcessSlotsAreBusy() throws Exception {
        properties.setMaxConcurrent(1);
        CountDownLatch entered = new CountDownLatch(1);
        CountDownLatch release = new CountDownLatch(1);
        ProcessExecutor executor = (command, timeout) -> {
            entered.countDown();
            try {
                if (!release.await(3, TimeUnit.SECONDS)) {
                    throw new ProcessTimeoutException("test release timeout", null);
                }
            } catch (InterruptedException exception) {
                Thread.currentThread().interrupt();
                throw new ProcessTimeoutException("interrupted", exception);
            }
            return new ProcessOutput(
                    0,
                    "{\"result\":\"10\",\"exact\":true,"
                            + "\"truncated\":false,\"repeating\":false}",
                    ""
            );
        };
        CliRadixEngine engine = engine(executor);

        try (ExecutorService caller = Executors.newVirtualThreadPerTaskExecutor()) {
            Future<EngineResult> first = caller.submit(() -> engine.convert(
                    new ConversionCommand("2", 10, 2, 256)));
            assertThat(entered.await(1, TimeUnit.SECONDS)).isTrue();

            assertThatThrownBy(() -> engine.convert(
                    new ConversionCommand("3", 10, 2, 256)))
                    .isInstanceOf(EngineBusyException.class);

            release.countDown();
            assertThat(first.get(2, TimeUnit.SECONDS).result()).isEqualTo("10");
        }
    }
}
