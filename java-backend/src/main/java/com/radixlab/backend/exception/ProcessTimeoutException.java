package com.radixlab.backend.exception;

public class ProcessTimeoutException extends RuntimeException {

    public ProcessTimeoutException(String message, Throwable cause) {
        super(message, cause);
    }
}
