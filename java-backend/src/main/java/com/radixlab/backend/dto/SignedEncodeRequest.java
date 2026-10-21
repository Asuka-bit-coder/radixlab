package com.radixlab.backend.dto;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

public record SignedEncodeRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 64, message = "must contain at most 64 characters")
        @Pattern(
                regexp = "[+-]?[0-9]+",
                message = "must be a decimal integer"
        )
        String value,

        @NotNull(message = "is required")
        @Min(value = 1, message = "must be between 1 and 64")
        @Max(value = 64, message = "must be between 1 and 64")
        Integer width,

        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "(?i)(SIGN[-_ ]?MAGNITUDE|ONES[-_ ]?COMPLEMENT|TWOS[-_ ]?COMPLEMENT)",
                message = "must be SIGN_MAGNITUDE, ONES_COMPLEMENT, or TWOS_COMPLEMENT"
        )
        String encoding,

        Boolean negativeZero
) {
}
