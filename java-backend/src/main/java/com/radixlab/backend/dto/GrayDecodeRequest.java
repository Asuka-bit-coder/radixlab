package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

public record GrayDecodeRequest(
        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "[01]{1,64}",
                message = "must contain 1 to 64 binary digits"
        )
        String bits
) {
}
