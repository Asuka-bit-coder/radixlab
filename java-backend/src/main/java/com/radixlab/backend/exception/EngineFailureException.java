package com.radixlab.backend.exception;

public class EngineFailureException extends RuntimeException {

    public EngineFailureException(String message, Throwable cause) {
        super(message, cause);
    }
}
