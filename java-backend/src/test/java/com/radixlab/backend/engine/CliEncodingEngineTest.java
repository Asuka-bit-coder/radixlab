package com.radixlab.backend.engine;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.radixlab.backend.config.EngineProperties;
import com.radixlab.backend.exception.EngineFailureException;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.InvalidEncodingException;
import com.radixlab.backend.exception.ProcessTimeoutException;
import com.radixlab.backend.exception.UnrepresentableValueException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class CliEncodingEngineTest {

    private EngineProperties properties;

    @BeforeEach
    void setUp() {
        properties = new EngineProperties();
        properties.setCliPath("/opt/radixlab/radixlab-cli");
        properties.setTimeout(Duration.ofSeconds(3));
        properties.setMaxConcurrent(2);
    }

    private CliEncodingEngine engine(ProcessExecutor executor) {
        return new CliEncodingEngine(
                new CliEngineClient(executor, properties),
                new ObjectMapper()
        );
    }

    @Test
    void encodesSignedAndRequestsNegativeZero() {
        AtomicReference<List<String>> captured = new AtomicReference<>();
        ProcessExecutor executor = (command, timeout) -> {
            captured.set(command);
            return new ProcessOutput(
                    0,
                    """
                    {"operation":"signed-encode","value":"0","width":8,
                     "encoding":"SIGN_MAGNITUDE","bits":"10000000",
                     "hex":"80","representable":true,"negativeZero":true,
                     "minimum":"-127","maximum":"127"}
                    """,
                    ""
            );
        };

        SignedEncodingResult result = engine(executor).encodeSigned(
                "0", 8, "SIGN_MAGNITUDE", true);

        assertThat(result.bits()).isEqualTo("10000000");
        assertThat(result.negativeZero()).isTrue();
        assertThat(result.minimum()).isEqualTo("-127");
        assertThat(captured.get()).containsExactly(
                "/opt/radixlab/radixlab-cli",
                "encoding",
                "--operation", "signed-encode",
                "--value", "0",
                "--width", "8",
                "--encoding", "SIGN_MAGNITUDE",
                "--negative-zero",
                "--json"
        );
    }

    @Test
    void mapsUnrepresentableRangeToDedicatedException() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                4,
                "",
                """
                {"error":{"code":"range_error",
                 "message":"Value -128 is outside the representable range [-127, 127]"}}
                """
        );

        assertThatThrownBy(() -> engine(executor).encodeSigned(
                "-128", 8, "SIGN_MAGNITUDE", false))
                .isInstanceOf(UnrepresentableValueException.class)
                .hasMessageContaining("outside the representable range");
    }

    @Test
    void mapsInvalidInputToEncodingException() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                2,
                "",
                """
                {"error":{"code":"invalid_input",
                 "message":"Illegal BCD nibble: each group must be 0000 through 1001"}}
                """
        );

        assertThatThrownBy(() -> engine(executor).decodeBcd("1010"))
                .isInstanceOf(InvalidEncodingException.class)
                .hasMessageContaining("Illegal BCD nibble");
    }

    @Test
    void parsesBcdGroups() {
        ProcessExecutor executor = (command, timeout) -> new ProcessOutput(
                0,
                """
                {"operation":"bcd-encode","decimal":"59","digits":"59",
                 "bits":"01011001","groups":["0101","1001"],
                 "value":"59","binary":"111011"}
                """,
                ""
        );

        BcdEncodingResult result = engine(executor).encodeBcd("59");

        assertThat(result.groups()).containsExactly("0101", "1001");
        assertThat(result.binary()).isEqualTo("111011");
    }

    @Test
    void passesOptionalParityFlipIndex() {
        AtomicReference<List<String>> captured = new AtomicReference<>();
        ProcessExecutor executor = (command, timeout) -> {
            captured.set(command);
            return new ProcessOutput(
                    0,
                    """
                    {"operation":"parity","bits":"1010001",
                     "originalBits":"1011001","width":7,"parity":"EVEN",
                     "onesCount":4,"receivedOnesCount":3,"parityBit":"0",
                     "computedParityBit":"1","dataWithParity":"10110010",
                     "receivedWithParity":"10100010","valid":false,
                     "flipped":true,"flippedIndex":3}
                    """,
                    ""
            );
        };

        ParityAnalysisResult result = engine(executor).analyzeParity(
                "1011001", "EVEN", 3);

        assertThat(result.valid()).isFalse();
        assertThat(result.flippedIndex()).isEqualTo(3);
        assertThat(captured.get()).containsSubsequence(
                "--operation", "parity",
                "--flip-bit", "3",
                "--json"
        );
    }

    @Test
    void rejectsMalformedEncodingJson() {
        ProcessExecutor executor = (command, timeout) ->
                new ProcessOutput(0, "{\"operation\":\"parity\"}", "");

        assertThatThrownBy(() -> engine(executor).decodeGray("1010"))
                .isInstanceOf(EngineFailureException.class)
                .hasMessage("Encoding engine returned an invalid response");
    }

    @Test
    void mapsEncodingProcessTimeout() {
        ProcessExecutor executor = (command, timeout) -> {
            throw new ProcessTimeoutException("timed out", null);
        };

        assertThatThrownBy(() -> engine(executor).encodeBcd("59"))
                .isInstanceOf(EngineTimeoutException.class)
                .hasMessage("Conversion engine timed out");
    }
}
