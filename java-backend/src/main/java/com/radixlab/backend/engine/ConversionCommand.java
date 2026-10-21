package com.radixlab.backend.engine;

public record ConversionCommand(
        String number,
        int fromBase,
        int toBase,
        int maxFractionDigits
) {
}
