package com.radixlab.backend.dto;

public record ParityResponse(
        String bits,
        String originalBits,
        int width,
        String parity,
        int onesCount,
        int receivedOnesCount,
        String parityBit,
        String computedParityBit,
        String dataWithParity,
        String receivedWithParity,
        boolean valid,
        boolean flipped,
        Integer flippedIndex
) {
}
