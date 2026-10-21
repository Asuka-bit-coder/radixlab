package com.radixlab.backend.engine;

public record EngineResult(
        String result,
        boolean exact,
        boolean truncated,
        boolean repeating
) {
}
