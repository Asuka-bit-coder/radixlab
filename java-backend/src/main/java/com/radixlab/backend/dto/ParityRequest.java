package com.radixlab.backend.dto;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

public record ParityRequest(
        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "[01]{1,64}",
                message = "must contain 1 to 64 binary digits"
        )
        String bits,

        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "(?i)(EVEN|ODD)(_?PARITY)?",
                message = "must be EVEN or ODD"
        )
        String parity,

        @Min(value = 0, message = "must be between 0 and 63")
        @Max(value = 63, message = "must be between 0 and 63")
        Integer flipBit
) {
}
