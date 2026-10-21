package com.radixlab.backend.controller;

import com.radixlab.backend.dto.ConvertRequest;
import com.radixlab.backend.dto.ConvertResponse;
import com.radixlab.backend.service.ConversionService;
import jakarta.validation.Valid;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1")
public class ConversionController {

    private final ConversionService conversionService;

    public ConversionController(ConversionService conversionService) {
        this.conversionService = conversionService;
    }

    @PostMapping(
            path = "/convert",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public ConvertResponse convert(@Valid @RequestBody ConvertRequest request) {
        return conversionService.convert(request);
    }
}
