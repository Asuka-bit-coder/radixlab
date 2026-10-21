package com.radixlab.backend.engine;

public record ProcessOutput(
        int exitCode,
        String stdout,
        String stderr
) {
}
