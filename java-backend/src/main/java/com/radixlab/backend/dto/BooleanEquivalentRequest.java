package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;

public record BooleanEquivalentRequest(
        @NotBlank(message = "must not be blank")
        String leftExpression,

        @NotBlank(message = "must not be blank")
        String rightExpression
) {
}
