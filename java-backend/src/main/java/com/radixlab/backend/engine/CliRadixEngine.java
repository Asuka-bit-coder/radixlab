package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.InvalidNumberException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.List;

@Component
public class CliRadixEngine implements RadixEngine {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(CliRadixEngine.class);

    private final CliEngineClient client;
    private final ObjectMapper objectMapper;

    public CliRadixEngine(
            CliEngineClient client,
            ObjectMapper objectMapper
    ) {
        this.client = client;
        this.objectMapper = objectMapper;
    }

    @Override
    public EngineResult convert(ConversionCommand command) {
        ProcessOutput output = client.execute(List.of(
                "--input", command.number(),
                "--from", Integer.toString(command.fromBase()),
                "--to", Integer.toString(command.toBase()),
                "--max-fraction-digits",
                Integer.toString(command.maxFractionDigits()),
                "--json"
        ));
        return interpret(output);
    }

    private EngineResult interpret(ProcessOutput output) {
        if (output.exitCode() == 2 || output.exitCode() == 3) {
            throw new InvalidNumberException(
                    CliJsonSupport.engineErrorMessage(
                            output,
                            objectMapper,
                            "The numeral is invalid for the selected source base"
                    )
            );
        }

        if (output.exitCode() != 0) {
            LOGGER.error(
                    "Conversion engine failed with exit code {}: {}",
                    output.exitCode(),
                    CliJsonSupport.diagnostic(output.stderr())
            );
            throw new EngineFailureException(
                    "Conversion engine failed", null);
        }

        try {
            JsonNode root = CliJsonSupport.requireObject(
                    output.stdout(), objectMapper);

            return new EngineResult(
                    CliJsonSupport.requiredText(root, "result"),
                    CliJsonSupport.requiredBoolean(root, "exact"),
                    CliJsonSupport.requiredBoolean(root, "truncated"),
                    CliJsonSupport.requiredBoolean(root, "repeating")
            );
        } catch (IOException exception) {
            LOGGER.error(
                    "Conversion engine returned invalid JSON: {}",
                    CliJsonSupport.diagnostic(output.stdout()),
                    exception
            );
            throw new EngineFailureException(
                    "Conversion engine returned an invalid response",
                    exception);
        }
    }
}
