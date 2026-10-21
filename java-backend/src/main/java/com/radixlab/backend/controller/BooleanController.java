package com.radixlab.backend.controller;

import com.radixlab.backend.dto.BooleanCanonicalResponse;
import com.radixlab.backend.dto.BooleanEquivalentRequest;
import com.radixlab.backend.dto.BooleanEquivalentResponse;
import com.radixlab.backend.dto.BooleanEvaluateRequest;
import com.radixlab.backend.dto.BooleanEvaluateResponse;
import com.radixlab.backend.dto.BooleanExpressionRequest;
import com.radixlab.backend.dto.BooleanTruthTableResponse;
import com.radixlab.backend.service.BooleanService;
import jakarta.validation.Valid;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1/boolean")
public class BooleanController {

    private final BooleanService booleanService;

    public BooleanController(BooleanService booleanService) {
        this.booleanService = booleanService;
    }

    @PostMapping(
            path = "/evaluate",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BooleanEvaluateResponse evaluate(
            @Valid @RequestBody BooleanEvaluateRequest request
    ) {
        return booleanService.evaluate(request);
    }

    @PostMapping(
            path = "/truth-table",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BooleanTruthTableResponse truthTable(
            @Valid @RequestBody BooleanExpressionRequest request
    ) {
        return booleanService.truthTable(request);
    }

    @PostMapping(
            path = "/canonical",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BooleanCanonicalResponse canonicalForms(
            @Valid @RequestBody BooleanExpressionRequest request
    ) {
        return booleanService.canonicalForms(request);
    }

    @PostMapping(
            path = "/equivalent",
            consumes = MediaType.APPLICATION_JSON_VALUE,
            produces = MediaType.APPLICATION_JSON_VALUE
    )
    public BooleanEquivalentResponse equivalent(
            @Valid @RequestBody BooleanEquivalentRequest request
    ) {
        return booleanService.equivalent(request);
    }
}
