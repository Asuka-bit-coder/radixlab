package com.radixlab.backend.engine;

import com.radixlab.backend.exception.ProcessStartException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.util.List;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class DefaultProcessExecutorTest {

    private final DefaultProcessExecutor executor =
            new DefaultProcessExecutor();

    @Test
    void capturesStdoutStderrAndExitCode() {
        ProcessOutput output = executor.execute(
                List.of("sh", "-c", "printf out; printf err >&2; exit 7"),
                Duration.ofSeconds(2)
        );

        assertThat(output.exitCode()).isEqualTo(7);
        assertThat(output.stdout()).isEqualTo("out");
        assertThat(output.stderr()).isEqualTo("err");
    }

    @Test
    void terminatesAProcessThatExceedsItsTimeout() {
        long start = System.nanoTime();

        assertThatThrownBy(() -> executor.execute(
                List.of("sleep", "5"),
                Duration.ofMillis(150)
        )).isInstanceOf(ProcessTimeoutException.class);

        long elapsedMillis = Duration.ofNanos(
                System.nanoTime() - start).toMillis();
        assertThat(elapsedMillis).isLessThan(3000);
    }

    @Test
    void reportsAnExecutableThatCannotStart() {
        assertThatThrownBy(() -> executor.execute(
                List.of("/definitely/not/a/radixlab-binary"),
                Duration.ofSeconds(1)
        )).isInstanceOf(ProcessStartException.class);
    }
}
