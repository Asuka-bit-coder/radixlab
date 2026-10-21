package com.radixlab.backend.engine;

public record GrayEncodingResult(
        String value,
        int width,
        String binary,
        String gray,
        String binaryHex,
        String grayHex
) {
}
