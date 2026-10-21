package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;

public record BooleanExpressionRequest(
        @NotBlank(message = "must not be blank")
        String expression
) {
}
