package com.radixlab.backend.service;

import com.radixlab.backend.dto.BcdEncodeRequest;
import com.radixlab.backend.dto.BcdEncodingResponse;
import com.radixlab.backend.dto.GrayEncodeRequest;
import com.radixlab.backend.dto.GrayEncodingResponse;
import com.radixlab.backend.dto.ParityRequest;
import com.radixlab.backend.dto.ParityResponse;
import com.radixlab.backend.dto.SignedEncodeRequest;
import com.radixlab.backend.dto.SignedEncodingResponse;
import com.radixlab.backend.engine.BcdEncodingResult;
import com.radixlab.backend.engine.EncodingEngine;
import com.radixlab.backend.engine.GrayEncodingResult;
import com.radixlab.backend.engine.ParityAnalysisResult;
import com.radixlab.backend.engine.SignedEncodingResult;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import java.util.List;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class EncodingServiceTest {

    @Mock
    private EncodingEngine engine;

    @InjectMocks
    private EncodingService service;

    @Test
    void mapsSignedEncodingAndNegativeZeroFlag() {
        when(engine.encodeSigned("0", 8, "SIGN_MAGNITUDE", true))
                .thenReturn(new SignedEncodingResult(
                        "0", 8, "SIGN_MAGNITUDE", "10000000", "80",
                        true, true, "-127", "127"));

        SignedEncodingResponse response = service.encodeSigned(
                new SignedEncodeRequest("0", 8, "SIGN_MAGNITUDE", true));

        assertThat(response.bits()).isEqualTo("10000000");
        assertThat(response.negativeZero()).isTrue();
        assertThat(response.maximum()).isEqualTo("127");
        verify(engine).encodeSigned("0", 8, "SIGN_MAGNITUDE", true);
    }

    @Test
    void mapsBcdResult() {
        when(engine.encodeBcd("59")).thenReturn(new BcdEncodingResult(
                "59", "59", "01011001",
                List.of("0101", "1001"), "59", "111011"));

        BcdEncodingResponse response = service.encodeBcd(
                new BcdEncodeRequest("59"));

        assertThat(response.groups()).containsExactly("0101", "1001");
        assertThat(response.binary()).isEqualTo("111011");
    }

    @Test
    void mapsGrayResult() {
        when(engine.encodeGray("13", 8)).thenReturn(new GrayEncodingResult(
                "13", 8, "00001101", "00001011", "0D", "0B"));

        GrayEncodingResponse response = service.encodeGray(
                new GrayEncodeRequest("13", 8));

        assertThat(response.gray()).isEqualTo("00001011");
        assertThat(response.grayHex()).isEqualTo("0B");
    }

    @Test
    void mapsParityResult() {
        when(engine.analyzeParity("1011001", "EVEN", 3))
                .thenReturn(new ParityAnalysisResult(
                        "1010001", "1011001", 7, "EVEN",
                        4, 3, "0", "1", "10110010",
                        "10100010", false, true, 3));

        ParityResponse response = service.analyzeParity(
                new ParityRequest("1011001", "EVEN", 3));

        assertThat(response.valid()).isFalse();
        assertThat(response.flippedIndex()).isEqualTo(3);
        assertThat(response.originalBits()).isEqualTo("1011001");
    }
}
