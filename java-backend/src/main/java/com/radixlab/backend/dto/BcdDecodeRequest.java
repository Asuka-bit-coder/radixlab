package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

public record BcdDecodeRequest(
        @NotBlank(message = "must not be blank")
        @Pattern(
                regexp = "(?:[01]{4}){1,1024}",
                message = "must contain 1 to 1024 complete 4-bit groups"
        )
        String bits
) {
}
