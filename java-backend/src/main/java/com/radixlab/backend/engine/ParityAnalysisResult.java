package com.radixlab.backend.engine;

public record ParityAnalysisResult(
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
