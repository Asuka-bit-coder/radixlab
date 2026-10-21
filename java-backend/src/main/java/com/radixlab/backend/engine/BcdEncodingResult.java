package com.radixlab.backend.engine;

import java.util.List;

public record BcdEncodingResult(
        String decimal,
        String digits,
        String bits,
        List<String> groups,
        String value,
        String binary
) {
}
