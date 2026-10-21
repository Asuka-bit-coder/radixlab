package com.radixlab.backend.service;

import com.radixlab.backend.dto.BcdDecodeRequest;
import com.radixlab.backend.dto.BcdEncodeRequest;
import com.radixlab.backend.dto.BcdEncodingResponse;
import com.radixlab.backend.dto.GrayDecodeRequest;
import com.radixlab.backend.dto.GrayEncodeRequest;
import com.radixlab.backend.dto.GrayEncodingResponse;
import com.radixlab.backend.dto.ParityRequest;
import com.radixlab.backend.dto.ParityResponse;
import com.radixlab.backend.dto.SignedDecodeRequest;
import com.radixlab.backend.dto.SignedEncodeRequest;
import com.radixlab.backend.dto.SignedEncodingResponse;
import com.radixlab.backend.engine.BcdEncodingResult;
import com.radixlab.backend.engine.EncodingEngine;
import com.radixlab.backend.engine.GrayEncodingResult;
import com.radixlab.backend.engine.ParityAnalysisResult;
import com.radixlab.backend.engine.SignedEncodingResult;
import org.springframework.stereotype.Service;

@Service
public class EncodingService {

    private final EncodingEngine engine;

    public EncodingService(EncodingEngine engine) {
        this.engine = engine;
    }

    public SignedEncodingResponse encodeSigned(SignedEncodeRequest request) {
        SignedEncodingResult result = engine.encodeSigned(
                request.value(),
                request.width(),
                request.encoding(),
                Boolean.TRUE.equals(request.negativeZero())
        );
        return signedResponse(result);
    }

    public SignedEncodingResponse decodeSigned(SignedDecodeRequest request) {
        return signedResponse(
                engine.decodeSigned(request.bits(), request.encoding()));
    }

    public BcdEncodingResponse encodeBcd(BcdEncodeRequest request) {
        return bcdResponse(engine.encodeBcd(request.decimal()));
    }

    public BcdEncodingResponse decodeBcd(BcdDecodeRequest request) {
        return bcdResponse(engine.decodeBcd(request.bits()));
    }

    public GrayEncodingResponse encodeGray(GrayEncodeRequest request) {
        return grayResponse(
                engine.encodeGray(request.value(), request.width()));
    }

    public GrayEncodingResponse decodeGray(GrayDecodeRequest request) {
        return grayResponse(engine.decodeGray(request.bits()));
    }

    public ParityResponse analyzeParity(ParityRequest request) {
        ParityAnalysisResult result = engine.analyzeParity(
                request.bits(),
                request.parity(),
                request.flipBit()
        );
        return new ParityResponse(
                result.bits(),
                result.originalBits(),
                result.width(),
                result.parity(),
                result.onesCount(),
                result.receivedOnesCount(),
                result.parityBit(),
                result.computedParityBit(),
                result.dataWithParity(),
                result.receivedWithParity(),
                result.valid(),
                result.flipped(),
                result.flippedIndex()
        );
    }

    private static SignedEncodingResponse signedResponse(
            SignedEncodingResult result
    ) {
        return new SignedEncodingResponse(
                result.value(),
                result.width(),
                result.encoding(),
                result.bits(),
                result.hex(),
                result.representable(),
                result.negativeZero(),
                result.minimum(),
                result.maximum()
        );
    }

    private static BcdEncodingResponse bcdResponse(BcdEncodingResult result) {
        return new BcdEncodingResponse(
                result.decimal(),
                result.digits(),
                result.bits(),
                result.groups(),
                result.value(),
                result.binary()
        );
    }

    private static GrayEncodingResponse grayResponse(
            GrayEncodingResult result
    ) {
        return new GrayEncodingResponse(
                result.value(),
                result.width(),
                result.binary(),
                result.gray(),
                result.binaryHex(),
                result.grayHex()
        );
    }
}
