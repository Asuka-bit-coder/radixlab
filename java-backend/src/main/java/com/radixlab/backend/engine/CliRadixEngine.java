package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.EngineBusyException;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.InvalidNumberException;
import com.radixlab.backend.exception.ProcessStartException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@Component
public class CliRadixEngine implements RadixEngine {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(CliRadixEngine.class);
    private static final long ACQUIRE_TIMEOUT_MILLIS = 500;
    private static final int MAX_DIAGNOSTIC_LENGTH = 300;

    private final ProcessExecutor processExecutor;
    private final ObjectMapper objectMapper;
    private final EngineProperties properties;
    private final Semaphore processSlots;

    public CliRadixEngine(
            ProcessExecutor processExecutor,
            ObjectMapper objectMapper,
            EngineProperties properties
    ) {
        this.processExecutor = processExecutor;
        this.objectMapper = objectMapper;
        this.properties = properties;
        this.processSlots = new Semaphore(
                Math.max(1, properties.getMaxConcurrent()), true);
    }

    @Override
    public EngineResult convert(ConversionCommand command) {
        final boolean acquired;
        try {
            acquired = processSlots.tryAcquire(
                    ACQUIRE_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            throw new EngineFailureException(
                    "Interrupted while waiting for conversion capacity",
                    exception);
        }

        if (!acquired) {
            throw new EngineBusyException(
                    "Conversion engine is at capacity; retry shortly");
        }

        try {
            List<String> invocation = List.of(
                    properties.getCliPath(),
                    "--input", command.number(),
                    "--from", Integer.toString(command.fromBase()),
                    "--to", Integer.toString(command.toBase()),
                    "--max-fraction-digits",
                    Integer.toString(command.maxFractionDigits()),
                    "--json"
            );

            ProcessOutput output = processExecutor.execute(
                    invocation, properties.getTimeout());
            return interpret(output);
        } catch (ProcessTimeoutException exception) {
            LOGGER.warn("Conversion engine timed out");
            throw new EngineTimeoutException(
                    "Conversion engine timed out", exception);
        } catch (ProcessStartException exception) {
            LOGGER.error("Conversion engine could not be executed", exception);
            throw new EngineFailureException(
                    "Conversion engine is unavailable", exception);
        } finally {
            processSlots.release();
        }
    }

    private EngineResult interpret(ProcessOutput output) {
        if (output.exitCode() == 2 || output.exitCode() == 3) {
            throw new InvalidNumberException(
                    extractEngineMessage(output.stderr()));
        }

        if (output.exitCode() != 0) {
            LOGGER.error(
                    "Conversion engine failed with exit code {}: {}",
                    output.exitCode(),
                    diagnostic(output.stderr())
            );
            throw new EngineFailureException(
                    "Conversion engine failed", null);
        }

        try {
            JsonNode root = objectMapper.readTree(output.stdout());
            JsonNode result = root.path("result");
            JsonNode exact = root.path("exact");
            JsonNode truncated = root.path("truncated");
            JsonNode repeating = root.path("repeating");

            if (!result.isTextual()
                    || !exact.isBoolean()
                    || !truncated.isBoolean()
                    || !repeating.isBoolean()) {
                throw new IOException("Engine JSON is missing required fields");
            }

            return new EngineResult(
                    result.textValue(),
                    exact.booleanValue(),
                    truncated.booleanValue(),
                    repeating.booleanValue()
            );
        } catch (IOException exception) {
            LOGGER.error(
                    "Conversion engine returned invalid JSON: {}",
                    diagnostic(output.stdout()),
                    exception
            );
            throw new EngineFailureException(
                    "Conversion engine returned an invalid response",
                    exception);
        }
    }

    private String extractEngineMessage(String stderr) {
        try {
            JsonNode root = objectMapper.readTree(stderr);
            String message = root.path("error").path("message").asText("");
            if (!message.isBlank()) {
                return message;
            }
        } catch (IOException ignored) {
            // Fall through to a stable, non-sensitive client message.
        }

        return "The numeral is invalid for the selected source base";
    }

    private static String diagnostic(String text) {
        String singleLine = text == null
                ? ""
                : text.replaceAll("[\\r\\n]+", " ").strip();
        return singleLine.length() <= MAX_DIAGNOSTIC_LENGTH
                ? singleLine
                : singleLine.substring(0, MAX_DIAGNOSTIC_LENGTH) + "...";
    }
}
