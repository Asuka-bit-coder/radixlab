package com.radixlab.backend.dto;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

public record GrayEncodeRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 64, message = "must contain at most 64 characters")
        @Pattern(
                regexp = "[0-9]+",
                message = "must be an unsigned decimal integer"
        )
        String value,

        @NotNull(message = "is required")
        @Min(value = 1, message = "must be between 1 and 64")
        @Max(value = 64, message = "must be between 1 and 64")
        Integer width
) {
}
