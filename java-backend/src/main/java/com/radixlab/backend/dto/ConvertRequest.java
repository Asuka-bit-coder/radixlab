package com.radixlab.backend.dto;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

public record ConvertRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 4096, message = "must contain at most 4096 characters")
        @Pattern(
                regexp = "^(?=.*[0-9A-Za-z])[+-]?[0-9A-Za-z]*(\\.[0-9A-Za-z]*)?$",
                message = "must be a numeral with at most one radix point"
        )
        String number,

        @NotNull(message = "is required")
        @Min(value = 2, message = "must be between 2 and 36")
        @Max(value = 36, message = "must be between 2 and 36")
        Integer fromBase,

        @NotNull(message = "is required")
        @Min(value = 2, message = "must be between 2 and 36")
        @Max(value = 36, message = "must be between 2 and 36")
        Integer toBase,

        @Min(value = 0, message = "must be between 0 and 4096")
        @Max(value = 4096, message = "must be between 0 and 4096")
        Integer maxFractionDigits
) {
}
