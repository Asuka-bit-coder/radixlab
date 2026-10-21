package com.radixlab.backend.dto;

import com.fasterxml.jackson.annotation.JsonInclude;

import java.time.Instant;

@JsonInclude(JsonInclude.Include.NON_NULL)
public record ApiError(
        Instant timestamp,
        int status,
        String error,
        String message,
        String path,
        String code,
        Integer position,
        Integer column
) {
    public ApiError(
            Instant timestamp,
            int status,
            String error,
            String message,
            String path
    ) {
        this(timestamp, status, error, message, path, null, null, null);
    }
}
