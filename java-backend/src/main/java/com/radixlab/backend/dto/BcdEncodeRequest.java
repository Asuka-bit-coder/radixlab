package com.radixlab.backend.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

public record BcdEncodeRequest(
        @NotBlank(message = "must not be blank")
        @Size(max = 1024, message = "must contain at most 1024 digits")
        @Pattern(
                regexp = "[0-9]+",
                message = "must contain decimal digits 0-9 only"
        )
        String decimal
) {
}
