package com.radixlab.backend.engine;

import com.radixlab.backend.exception.ProcessStartException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Starts the CLI directly, without a shell. stdout and stderr are drained
 * concurrently on virtual threads so a child process cannot deadlock.
 */
@Component
public class DefaultProcessExecutor implements ProcessExecutor {

    private static final Duration DRAIN_TIMEOUT = Duration.ofSeconds(1);
    private static final Duration TERMINATION_GRACE = Duration.ofMillis(300);

    @Override
    public ProcessOutput execute(List<String> command, Duration timeout) {
        Objects.requireNonNull(command, "command");
        Objects.requireNonNull(timeout, "timeout");
        if (command.isEmpty()) {
            throw new IllegalArgumentException("command must not be empty");
        }

        final Process process;
        try {
            process = new ProcessBuilder(command)
                    .redirectErrorStream(false)
                    .start();
        } catch (IOException exception) {
            throw new ProcessStartException(
                    "Unable to start conversion engine", exception);
        }

        try (ExecutorService drainer = Executors.newVirtualThreadPerTaskExecutor()) {
            Future<String> stdout = drainer.submit(
                    () -> readUtf8(process.getInputStream()));
            Future<String> stderr = drainer.submit(
                    () -> readUtf8(process.getErrorStream()));

            final boolean finished;
            try {
                finished = process.waitFor(
                        Math.max(1, timeout.toMillis()), TimeUnit.MILLISECONDS);
            } catch (InterruptedException exception) {
                terminate(process);
                Thread.currentThread().interrupt();
                throw new ProcessStartException(
                        "Interrupted while waiting for conversion engine",
                        exception);
            }

            if (!finished) {
                terminate(process);
                throw new ProcessTimeoutException(
                        "Conversion engine exceeded its time limit", null);
            }

            return new ProcessOutput(
                    process.exitValue(),
                    await(stdout),
                    await(stderr)
            );
        }
    }

    private static String readUtf8(InputStream stream) throws IOException {
        try (stream) {
            return new String(stream.readAllBytes(), StandardCharsets.UTF_8);
        }
    }

    private static String await(Future<String> future) {
        try {
            return future.get(DRAIN_TIMEOUT.toMillis(), TimeUnit.MILLISECONDS);
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            throw new ProcessStartException(
                    "Interrupted while reading conversion engine output",
                    exception);
        } catch (ExecutionException | TimeoutException exception) {
            throw new ProcessStartException(
                    "Unable to read conversion engine output", exception);
        }
    }

    private static void terminate(Process process) {
        process.destroy();
        try {
            if (!process.waitFor(
                    TERMINATION_GRACE.toMillis(), TimeUnit.MILLISECONDS)) {
                process.destroyForcibly();
                process.waitFor(
                        DRAIN_TIMEOUT.toMillis(), TimeUnit.MILLISECONDS);
            }
        } catch (InterruptedException exception) {
            process.destroyForcibly();
            Thread.currentThread().interrupt();
        }
    }
}
