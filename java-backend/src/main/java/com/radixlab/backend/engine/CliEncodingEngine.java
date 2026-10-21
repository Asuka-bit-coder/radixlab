package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.InvalidEncodingException;
import com.radixlab.backend.exception.UnrepresentableValueException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

@Component
public class CliEncodingEngine implements EncodingEngine {

    private static final Logger LOGGER =
            LoggerFactory.getLogger(CliEncodingEngine.class);

    private final CliEngineClient client;
    private final ObjectMapper objectMapper;

    public CliEncodingEngine(
            CliEngineClient client,
            ObjectMapper objectMapper
    ) {
        this.client = client;
        this.objectMapper = objectMapper;
    }

    @Override
    public SignedEncodingResult encodeSigned(
            String value,
            int width,
            String encoding,
            boolean negativeZero
    ) {
        List<String> arguments = new ArrayList<>(List.of(
                "encoding",
                "--operation", "signed-encode",
                "--value", value,
                "--width", Integer.toString(width),
                "--encoding", encoding
        ));
        if (negativeZero) {
            arguments.add("--negative-zero");
        }

        return interpretSigned(invoke(withJson(arguments)));
    }

    @Override
    public SignedEncodingResult decodeSigned(String bits, String encoding) {
        return interpretSigned(invoke(List.of(
                "encoding",
                "--operation", "signed-decode",
                "--bits", bits,
                "--encoding", encoding,
                "--json"
        )));
    }

    @Override
    public BcdEncodingResult encodeBcd(String decimal) {
        return interpretBcd(invoke(List.of(
                "encoding",
                "--operation", "bcd-encode",
                "--decimal", decimal,
                "--json"
        )));
    }

    @Override
    public BcdEncodingResult decodeBcd(String bits) {
        return interpretBcd(invoke(List.of(
                "encoding",
                "--operation", "bcd-decode",
                "--bits", bits,
                "--json"
        )));
    }

    @Override
    public GrayEncodingResult encodeGray(String value, int width) {
        return interpretGray(invoke(List.of(
                "encoding",
                "--operation", "gray-encode",
                "--value", value,
                "--width", Integer.toString(width),
                "--json"
        )));
    }

    @Override
    public GrayEncodingResult decodeGray(String bits) {
        return interpretGray(invoke(List.of(
                "encoding",
                "--operation", "gray-decode",
                "--bits", bits,
                "--json"
        )));
    }

    @Override
    public ParityAnalysisResult analyzeParity(
            String bits,
            String parity,
            Integer flipBit
    ) {
        List<String> arguments = new ArrayList<>(List.of(
                "encoding",
                "--operation", "parity",
                "--bits", bits,
                "--parity", parity
        ));
        if (flipBit != null) {
            arguments.add("--flip-bit");
            arguments.add(Integer.toString(flipBit));
        }

        return interpretParity(invoke(withJson(arguments)));
    }

    private static List<String> withJson(List<String> arguments) {
        List<String> invocation = new ArrayList<>(arguments.size() + 1);
        invocation.addAll(arguments);
        invocation.add("--json");
        return invocation;
    }

    private ProcessOutput invoke(List<String> arguments) {
        return client.execute(arguments);
    }

    private void validateExit(ProcessOutput output) {
        if (output.exitCode() == 4) {
            throw new UnrepresentableValueException(
                    CliJsonSupport.engineErrorMessage(
                            output,
                            objectMapper,
                            "The value is outside the representable range"
                    )
            );
        }

        if (output.exitCode() == 2 || output.exitCode() == 3) {
            throw new InvalidEncodingException(
                    CliJsonSupport.engineErrorMessage(
                            output,
                            objectMapper,
                            "The encoding request is invalid"
                    )
            );
        }

        if (output.exitCode() != 0) {
            LOGGER.error(
                    "Encoding engine failed with exit code {}: {}",
                    output.exitCode(),
                    CliJsonSupport.diagnostic(output.stderr())
            );
            throw new EngineFailureException(
                    "Encoding engine failed", null);
        }
    }

    private JsonNode successJson(ProcessOutput output) {
        validateExit(output);

        try {
            return CliJsonSupport.requireObject(
                    output.stdout(), objectMapper);
        } catch (IOException exception) {
            LOGGER.error(
                    "Encoding engine returned invalid JSON: {}",
                    CliJsonSupport.diagnostic(output.stdout()),
                    exception
            );
            throw new EngineFailureException(
                    "Encoding engine returned an invalid response",
                    exception);
        }
    }

    private SignedEncodingResult interpretSigned(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            return new SignedEncodingResult(
                    CliJsonSupport.requiredText(root, "value"),
                    CliJsonSupport.requiredInt(root, "width"),
                    CliJsonSupport.requiredText(root, "encoding"),
                    CliJsonSupport.requiredText(root, "bits"),
                    CliJsonSupport.requiredText(root, "hex"),
                    CliJsonSupport.requiredBoolean(root, "representable"),
                    CliJsonSupport.requiredBoolean(root, "negativeZero"),
                    CliJsonSupport.requiredText(root, "minimum"),
                    CliJsonSupport.requiredText(root, "maximum")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private BcdEncodingResult interpretBcd(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            JsonNode groupsNode = root.path("groups");
            if (!groupsNode.isArray()) {
                throw new IOException("Engine JSON is missing groups array");
            }

            List<String> groups = new ArrayList<>();
            for (JsonNode group : groupsNode) {
                if (!group.isTextual()) {
                    throw new IOException("BCD group is not text");
                }
                groups.add(group.textValue());
            }

            return new BcdEncodingResult(
                    CliJsonSupport.requiredText(root, "decimal"),
                    CliJsonSupport.requiredText(root, "digits"),
                    CliJsonSupport.requiredText(root, "bits"),
                    List.copyOf(groups),
                    CliJsonSupport.requiredText(root, "value"),
                    CliJsonSupport.requiredText(root, "binary")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private GrayEncodingResult interpretGray(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            return new GrayEncodingResult(
                    CliJsonSupport.requiredText(root, "value"),
                    CliJsonSupport.requiredInt(root, "width"),
                    CliJsonSupport.requiredText(root, "binary"),
                    CliJsonSupport.requiredText(root, "gray"),
                    CliJsonSupport.requiredText(root, "binaryHex"),
                    CliJsonSupport.requiredText(root, "grayHex")
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private ParityAnalysisResult interpretParity(ProcessOutput output) {
        JsonNode root = successJson(output);

        try {
            JsonNode indexNode = root.path("flippedIndex");
            Integer flippedIndex = null;
            if (!indexNode.isNull() && !indexNode.isMissingNode()) {
                if (!indexNode.isIntegralNumber()
                        || !indexNode.canConvertToInt()) {
                    throw new IOException(
                            "Engine JSON has an invalid flippedIndex");
                }
                flippedIndex = indexNode.intValue();
            }

            return new ParityAnalysisResult(
                    CliJsonSupport.requiredText(root, "bits"),
                    CliJsonSupport.requiredText(root, "originalBits"),
                    CliJsonSupport.requiredInt(root, "width"),
                    CliJsonSupport.requiredText(root, "parity"),
                    CliJsonSupport.requiredInt(root, "onesCount"),
                    CliJsonSupport.requiredInt(root, "receivedOnesCount"),
                    CliJsonSupport.requiredText(root, "parityBit"),
                    CliJsonSupport.requiredText(root, "computedParityBit"),
                    CliJsonSupport.requiredText(root, "dataWithParity"),
                    CliJsonSupport.requiredText(root, "receivedWithParity"),
                    CliJsonSupport.requiredBoolean(root, "valid"),
                    CliJsonSupport.requiredBoolean(root, "flipped"),
                    flippedIndex
            );
        } catch (IOException exception) {
            throw invalidResponse(output, exception);
        }
    }

    private EngineFailureException invalidResponse(
            ProcessOutput output,
            IOException cause
    ) {
        LOGGER.error(
                "Encoding engine returned an invalid response: {}",
                CliJsonSupport.diagnostic(output.stdout()),
                cause
        );
        return new EngineFailureException(
                "Encoding engine returned an invalid response",
                cause
        );
    }
}
