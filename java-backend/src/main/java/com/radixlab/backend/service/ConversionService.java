package com.radixlab.backend.service;

import com.radixlab.backend.dto.ConvertRequest;
import com.radixlab.backend.dto.ConvertResponse;
import com.radixlab.backend.engine.ConversionCommand;
import com.radixlab.backend.engine.EngineResult;
import com.radixlab.backend.engine.RadixEngine;
import org.springframework.stereotype.Service;

@Service
public class ConversionService {

    static final int DEFAULT_MAX_FRACTION_DIGITS = 256;

    private final RadixEngine engine;

    public ConversionService(RadixEngine engine) {
        this.engine = engine;
    }

    public ConvertResponse convert(ConvertRequest request) {
        int fractionLimit = request.maxFractionDigits() == null
                ? DEFAULT_MAX_FRACTION_DIGITS
                : request.maxFractionDigits();

        EngineResult result = engine.convert(new ConversionCommand(
                request.number(),
                request.fromBase(),
                request.toBase(),
                fractionLimit
        ));

        return new ConvertResponse(
                result.result(),
                result.exact(),
                result.truncated(),
                result.repeating()
        );
    }
}
