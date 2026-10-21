package com.radixlab.backend.dto;

public record SignedEncodingResponse(
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
