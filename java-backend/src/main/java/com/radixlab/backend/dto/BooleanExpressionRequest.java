package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

public record BooleanExpressionRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 1024, message = "must contain at most 1024 characters")
        String expression
) {
}
