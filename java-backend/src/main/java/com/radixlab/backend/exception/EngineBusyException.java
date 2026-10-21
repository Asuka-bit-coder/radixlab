package com.radixlab.backend.exception;

public class EngineBusyException extends RuntimeException {

    public EngineBusyException(String message) {
        super(message);
    }
}
