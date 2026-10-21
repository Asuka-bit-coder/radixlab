package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.IOException;

final class CliJsonSupport {

    private static final int MAX_DIAGNOSTIC_LENGTH = 300;

    private CliJsonSupport() {
    }

    static JsonNode requireObject(String stdout, ObjectMapper objectMapper)
            throws IOException {
        JsonNode root = objectMapper.readTree(stdout);
        if (root == null || !root.isObject()) {
            throw new IOException("Engine response is not a JSON object");
        }
        return root;
    }

    static String requiredText(JsonNode root, String field) throws IOException {
        JsonNode node = root.path(field);
        if (!node.isTextual()) {
            throw new IOException("Engine JSON is missing text field: " + field);
        }
        return node.textValue();
    }

    static int requiredInt(JsonNode root, String field) throws IOException {
        JsonNode node = root.path(field);
        if (!node.isIntegralNumber() || !node.canConvertToInt()) {
            throw new IOException("Engine JSON is missing integer field: " + field);
        }
        return node.intValue();
    }

    static boolean requiredBoolean(JsonNode root, String field)
            throws IOException {
        JsonNode node = root.path(field);
        if (!node.isBoolean()) {
            throw new IOException("Engine JSON is missing boolean field: " + field);
        }
        return node.booleanValue();
    }

    static String engineErrorMessage(
            ProcessOutput output,
            ObjectMapper objectMapper,
            String fallback
    ) {
        try {
            JsonNode root = objectMapper.readTree(output.stderr());
            if (root != null && root.isObject()) {
                String message = root.path("error").path("message").asText("");
                if (!message.isBlank()) {
                    return message;
                }
            }
        } catch (IOException ignored) {
            // Fall through to a stable, non-sensitive client message.
        }

        return fallback;
    }

    static String diagnostic(String text) {
        String singleLine = text == null
                ? ""
                : text.replaceAll("[\\r\\n]+", " ").strip();
        return singleLine.length() <= MAX_DIAGNOSTIC_LENGTH
                ? singleLine
                : singleLine.substring(0, MAX_DIAGNOSTIC_LENGTH) + "...";
    }
}
