package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

public record BooleanEquivalentRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 1024, message = "must contain at most 1024 characters")
        String leftExpression,

        @NotBlank(message = "must not be blank")
        @Size(max = 1024, message = "must contain at most 1024 characters")
        String rightExpression
) {
}
