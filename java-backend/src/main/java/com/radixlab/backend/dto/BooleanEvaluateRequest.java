package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

import java.util.Map;

public record BooleanEvaluateRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 1024, message = "must contain at most 1024 characters")
        String expression,

        @Size(max = 26, message = "must contain at most 26 assignments")
        Map<String, Boolean> assignment
) {
}
