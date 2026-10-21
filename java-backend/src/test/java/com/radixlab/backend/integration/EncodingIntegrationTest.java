package com.radixlab.backend.integration;

import com.radixlab.backend.dto.BcdDecodeRequest;
import com.radixlab.backend.dto.BcdEncodeRequest;
import com.radixlab.backend.dto.BcdEncodingResponse;
import com.radixlab.backend.dto.GrayDecodeRequest;
import com.radixlab.backend.dto.GrayEncodeRequest;
import com.radixlab.backend.dto.GrayEncodingResponse;
import com.radixlab.backend.dto.ParityRequest;
import com.radixlab.backend.dto.ParityResponse;
import com.radixlab.backend.dto.SignedDecodeRequest;
import com.radixlab.backend.dto.SignedEncodeRequest;
import com.radixlab.backend.dto.SignedEncodingResponse;
import org.junit.jupiter.api.Tag;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.web.client.TestRestTemplate;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;

import java.nio.file.Files;
import java.nio.file.Path;

import static org.assertj.core.api.Assertions.assertThat;

@Tag("integration")
@SpringBootTest(webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class EncodingIntegrationTest {

    private static final Path CLI_PATH = Path.of(System.getenv()
            .getOrDefault("RADIXLAB_CLI_PATH", "../cpp-core/build/radixlab-cli"))
            .toAbsolutePath()
            .normalize();

    @Autowired
    private TestRestTemplate restTemplate;

    @DynamicPropertySource
    static void engineProperties(DynamicPropertyRegistry registry) {
        if (!Files.isExecutable(CLI_PATH)) {
            throw new IllegalStateException(
                    "Integration test requires the C++ CLI at " + CLI_PATH
                            + ". Build cpp-core or set RADIXLAB_CLI_PATH.");
        }
        registry.add("radixlab.engine.cli-path", CLI_PATH::toString);
        registry.add("radixlab.engine.timeout", () -> "5s");
    }

    @Test
    void servesEncodingLabPage() {
        ResponseEntity<String> response = restTemplate.getForEntity(
                "/encoding.html", String.class);

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody())
                .contains("Encoding Lab")
                .contains("Signed number explorer")
                .contains("encoding.js");
    }

    @Test
    void encodesSignedValueThroughRealEngine() {
        ResponseEntity<SignedEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/signed",
                        new SignedEncodeRequest(
                                "-13", 8, "TWOS_COMPLEMENT", false),
                        SignedEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().bits()).isEqualTo("11110011");
        assertThat(response.getBody().hex()).isEqualTo("F3");
        assertThat(response.getBody().minimum()).isEqualTo("-128");
        assertThat(response.getBody().maximum()).isEqualTo("127");
    }

    @Test
    void encodesRequestedNegativeZeroThroughRealEngine() {
        ResponseEntity<SignedEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/signed",
                        new SignedEncodeRequest(
                                "0", 8, "SIGN_MAGNITUDE", true),
                        SignedEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().bits()).isEqualTo("10000000");
        assertThat(response.getBody().negativeZero()).isTrue();
        assertThat(response.getBody().value()).isEqualTo("0");
    }

    @Test
    void preservesNegativeZeroInDecodedPattern() {
        ResponseEntity<SignedEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/signed/decode",
                        new SignedDecodeRequest("10000000", "SIGN_MAGNITUDE"),
                        SignedEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().value()).isEqualTo("0");
        assertThat(response.getBody().negativeZero()).isTrue();
        assertThat(response.getBody().bits()).isEqualTo("10000000");
    }

    @Test
    void rejectsUnrepresentableSignedValueWith422() {
        ResponseEntity<String> response = restTemplate.postForEntity(
                "/api/v1/encoding/signed",
                new SignedEncodeRequest("-128", 8, "SIGN_MAGNITUDE", false),
                String.class
        );

        assertThat(response.getStatusCode())
                .isEqualTo(HttpStatus.UNPROCESSABLE_ENTITY);
        assertThat(response.getBody())
                .contains("outside the representable range")
                .doesNotContain(CLI_PATH.toString());
    }

    @Test
    void encodesBcdWithGroupsAndBinaryContrast() {
        ResponseEntity<BcdEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/bcd",
                        new BcdEncodeRequest("59"),
                        BcdEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().bits()).isEqualTo("01011001");
        assertThat(response.getBody().groups()).containsExactly("0101", "1001");
        assertThat(response.getBody().binary()).isEqualTo("111011");
    }

    @Test
    void decodesBcdThroughRealEngine() {
        ResponseEntity<BcdEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/bcd/decode",
                        new BcdDecodeRequest("01011001"),
                        BcdEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().digits()).isEqualTo("59");
        assertThat(response.getBody().binary()).isEqualTo("111011");
    }

    @Test
    void decodesGrayThroughRealEngine() {
        ResponseEntity<GrayEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/gray/decode",
                        new GrayDecodeRequest("00001011"),
                        GrayEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().value()).isEqualTo("13");
        assertThat(response.getBody().binary()).isEqualTo("00001101");
    }

    @Test
    void rejectsIllegalBcdNibble() {
        ResponseEntity<String> response = restTemplate.postForEntity(
                "/api/v1/encoding/bcd/decode",
                new BcdDecodeRequest("1010"),
                String.class
        );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.BAD_REQUEST);
        assertThat(response.getBody()).contains("Illegal BCD nibble");
    }

    @Test
    void encodesGrayCodeThroughRealEngine() {
        ResponseEntity<GrayEncodingResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/gray",
                        new GrayEncodeRequest("13", 8),
                        GrayEncodingResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().binary()).isEqualTo("00001101");
        assertThat(response.getBody().gray()).isEqualTo("00001011");
    }

    @Test
    void detectsParityErrorAfterSingleBitFlip() {
        ResponseEntity<ParityResponse> response =
                restTemplate.postForEntity(
                        "/api/v1/encoding/parity",
                        new ParityRequest("1011001", "EVEN", 3),
                        ParityResponse.class
                );

        assertThat(response.getStatusCode()).isEqualTo(HttpStatus.OK);
        assertThat(response.getBody()).isNotNull();
        assertThat(response.getBody().valid()).isFalse();
        assertThat(response.getBody().bits()).isEqualTo("1010001");
        assertThat(response.getBody().flippedIndex()).isEqualTo(3);
    }
}
