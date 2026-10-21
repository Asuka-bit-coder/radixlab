package com.radixlab.backend.exception;

public class EngineTimeoutException extends RuntimeException {

    public EngineTimeoutException(String message, Throwable cause) {
        super(message, cause);
    }
}
