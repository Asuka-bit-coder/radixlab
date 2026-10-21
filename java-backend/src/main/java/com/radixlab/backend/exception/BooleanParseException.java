package com.radixlab.backend.exception;

public class BooleanParseException extends RuntimeException {

    private final int position;
    private final int column;

    public BooleanParseException(String message, int position, int column) {
        super(message);
        this.position = position;
        this.column = column;
    }

    public int position() {
        return position;
    }

    public int column() {
        return column;
    }
}
