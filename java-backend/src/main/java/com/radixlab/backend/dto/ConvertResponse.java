package com.radixlab.backend.dto;

public record ConvertResponse(
        String result,
        boolean exact,
        boolean truncated,
        boolean repeating
) {
}
