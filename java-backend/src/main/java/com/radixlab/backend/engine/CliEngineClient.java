package com.radixlab.backend.engine;

import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.EngineBusyException;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.ProcessStartException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * Shared process boundary for every CLI-backed engine.
 *
 * A single semaphore and timeout policy cover conversion and encoding calls,
 * so adding an engine does not create a second ProcessBuilder infrastructure.
 */
@Component
public class CliEngineClient {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(CliEngineClient.class);
    private static final long ACQUIRE_TIMEOUT_MILLIS = 500;

    private final ProcessExecutor processExecutor;
    private final EngineProperties properties;
    private final Semaphore processSlots;

    public CliEngineClient(
            ProcessExecutor processExecutor,
            EngineProperties properties
    ) {
        this.processExecutor = processExecutor;
        this.properties = properties;
        this.processSlots = new Semaphore(
                Math.max(1, properties.getMaxConcurrent()), true);
    }

    public ProcessOutput execute(List<String> arguments) {
        Objects.requireNonNull(arguments, "arguments");

        if (properties.getCliPath() == null
                || properties.getCliPath().isBlank()) {
            throw new EngineFailureException(
                    "Conversion engine path is not configured", null);
        }

        if (!acquireSlot()) {
            throw new EngineBusyException(
                    "Conversion engine is at capacity; retry shortly");
        }

        try {
            List<String> invocation = new ArrayList<>(arguments.size() + 1);
            invocation.add(properties.getCliPath());
            invocation.addAll(arguments);

            return processExecutor.execute(invocation, properties.getTimeout());
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

    private boolean acquireSlot() {
        try {
            return processSlots.tryAcquire(
                    ACQUIRE_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            throw new EngineFailureException(
                    "Interrupted while waiting for conversion capacity",
                    exception);
        }
    }
}
