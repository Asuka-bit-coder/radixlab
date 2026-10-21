package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

public record SignedDecodeRequest(
        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "[01]{1,64}",
                message = "must contain 1 to 64 binary digits"
        )
        String bits,

        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "(?i)(SIGN[-_ ]?MAGNITUDE|ONES[-_ ]?COMPLEMENT|TWOS[-_ ]?COMPLEMENT)",
                message = "must be SIGN_MAGNITUDE, ONES_COMPLEMENT, or TWOS_COMPLEMENT"
        )
        String encoding
) {
}
