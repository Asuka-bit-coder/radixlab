package com.radixlab.backend.controller;

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
import com.radixlab.backend.service.EncodingService;
import jakarta.validation.Valid;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1/encoding")
public class EncodingController {

    private final EncodingService encodingService;

    public EncodingController(EncodingService encodingService) {
        this.encodingService = encodingService;
    }

    @PostMapping(
            path = "/signed",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public SignedEncodingResponse encodeSigned(
            @Valid @RequestBody SignedEncodeRequest request
    ) {
        return encodingService.encodeSigned(request);
    }

    @PostMapping(
            path = "/signed/decode",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public SignedEncodingResponse decodeSigned(
            @Valid @RequestBody SignedDecodeRequest request
    ) {
        return encodingService.decodeSigned(request);
    }

    @PostMapping(
            path = "/bcd",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BcdEncodingResponse encodeBcd(
            @Valid @RequestBody BcdEncodeRequest request
    ) {
        return encodingService.encodeBcd(request);
    }

    @PostMapping(
            path = "/bcd/decode",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BcdEncodingResponse decodeBcd(
            @Valid @RequestBody BcdDecodeRequest request
    ) {
        return encodingService.decodeBcd(request);
    }

    @PostMapping(
            path = "/gray",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public GrayEncodingResponse encodeGray(
            @Valid @RequestBody GrayEncodeRequest request
    ) {
        return encodingService.encodeGray(request);
    }

    @PostMapping(
            path = "/gray/decode",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public GrayEncodingResponse decodeGray(
            @Valid @RequestBody GrayDecodeRequest request
    ) {
        return encodingService.decodeGray(request);
    }

    @PostMapping(
            path = "/parity",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public ParityResponse analyzeParity(
            @Valid @RequestBody ParityRequest request
    ) {
        return encodingService.analyzeParity(request);
    }
}
