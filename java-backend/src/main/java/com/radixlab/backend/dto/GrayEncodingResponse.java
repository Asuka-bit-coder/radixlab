package com.radixlab.backend.dto;

public record GrayEncodingResponse(
        String value,
        int width,
        String binary,
        String gray,
        String binaryHex,
        String grayHex
) {
}
