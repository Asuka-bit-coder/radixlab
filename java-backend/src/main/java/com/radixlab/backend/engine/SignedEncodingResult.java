package com.radixlab.backend.engine;

public record SignedEncodingResult(
        String value,
        int width,
        String encoding,
        String bits,
        String hex,
        boolean representable,
        boolean negativeZero,
        String minimum,
        String maximum
) {
}
