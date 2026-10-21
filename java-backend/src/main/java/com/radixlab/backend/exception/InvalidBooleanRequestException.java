package com.radixlab.backend.exception;

public class InvalidBooleanRequestException extends RuntimeException {

    public InvalidBooleanRequestException(String message) {
        super(message);
    }
}
