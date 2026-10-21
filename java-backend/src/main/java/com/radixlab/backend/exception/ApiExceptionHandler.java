package com.radixlab.backend.exception;

import com.radixlab.backend.dto.ApiError;
import jakarta.servlet.http.HttpServletRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.time.Instant;

@RestControllerAdvice
public class ApiExceptionHandler {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(ApiExceptionHandler.class);

    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<ApiError> handleValidation(
            MethodArgumentNotValidException exception,
            HttpServletRequest request
    ) {
        String message = exception.getBindingResult()
                .getFieldErrors()
                .stream()
                .findFirst()
                .map(error -> error.getField() + " " + error.getDefaultMessage())
                .orElse("Request validation failed");

        return response(HttpStatus.BAD_REQUEST, message, request);
    }

    @ExceptionHandler(HttpMessageNotReadableException.class)
    public ResponseEntity<ApiError> handleUnreadableBody(
            HttpMessageNotReadableException exception,
            HttpServletRequest request
    ) {
        return response(
                HttpStatus.BAD_REQUEST,
                "Malformed JSON request",
                request
        );
    }

    @ExceptionHandler(InvalidNumberException.class)
    public ResponseEntity<ApiError> handleInvalidNumber(
            InvalidNumberException exception,
            HttpServletRequest request
    ) {
        return response(HttpStatus.BAD_REQUEST, exception.getMessage(), request);
    }

    @ExceptionHandler(EngineTimeoutException.class)
    public ResponseEntity<ApiError> handleTimeout(
            EngineTimeoutException exception,
            HttpServletRequest request
    ) {
        return response(
                HttpStatus.GATEWAY_TIMEOUT,
                "Conversion engine timed out",
                request
        );
    }

    @ExceptionHandler(EngineBusyException.class)
    public ResponseEntity<ApiError> handleBusy(
            EngineBusyException exception,
            HttpServletRequest request
    ) {
        return response(
                HttpStatus.SERVICE_UNAVAILABLE,
                "Conversion engine is busy; retry shortly",
                request
        );
    }

    @ExceptionHandler(EngineFailureException.class)
    public ResponseEntity<ApiError> handleEngineFailure(
            EngineFailureException exception,
            HttpServletRequest request
    ) {
        return response(
                HttpStatus.BAD_GATEWAY,
                "Conversion engine failed",
                request
        );
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiError> handleUnexpected(
            Exception exception,
            HttpServletRequest request
    ) {
        LOGGER.error("Unhandled API error", exception);
        return response(
                HttpStatus.INTERNAL_SERVER_ERROR,
                "Internal server error",
                request
        );
    }

    private static ResponseEntity<ApiError> response(
            HttpStatus status,
            String message,
            HttpServletRequest request
    ) {
        ApiError body = new ApiError(
                Instant.now(),
                status.value(),
                status.getReasonPhrase(),
                message,
                request.getRequestURI()
        );
        return ResponseEntity.status(status).body(body);
    }
}
