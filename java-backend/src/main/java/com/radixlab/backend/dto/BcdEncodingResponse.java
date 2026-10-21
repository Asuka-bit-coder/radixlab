package com.radixlab.backend.dto;

import java.util.List;

public record BcdEncodingResponse(
        String decimal,
        String digits,
        String bits,
        List<String> groups,
        String value,
        String binary
) {
}
