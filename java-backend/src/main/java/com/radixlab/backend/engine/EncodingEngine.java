package com.radixlab.backend.engine;

public interface EncodingEngine {

    SignedEncodingResult encodeSigned(
            String value,
            int width,
            String encoding,
            boolean negativeZero
    );

    SignedEncodingResult decodeSigned(String bits, String encoding);

    BcdEncodingResult encodeBcd(String decimal);

    BcdEncodingResult decodeBcd(String bits);

    GrayEncodingResult encodeGray(String value, int width);

    GrayEncodingResult decodeGray(String bits);

    ParityAnalysisResult analyzeParity(
            String bits,
            String parity,
            Integer flipBit
    );
}
