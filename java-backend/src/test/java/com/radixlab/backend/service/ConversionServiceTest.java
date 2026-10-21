package com.radixlab.backend.service;

import com.radixlab.backend.dto.ConvertRequest;
import com.radixlab.backend.dto.ConvertResponse;
import com.radixlab.backend.engine.ConversionCommand;
import com.radixlab.backend.engine.EngineResult;
import com.radixlab.backend.engine.RadixEngine;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class ConversionServiceTest {

    @Mock
    private RadixEngine engine;

    @InjectMocks
    private ConversionService service;

    @Test
    void mapsRequestAndEngineResult() {
        when(engine.convert(new ConversionCommand(
                "101101.011", 2, 16, 128)))
                .thenReturn(new EngineResult("2D.6", true, false, false));

        ConvertResponse response = service.convert(new ConvertRequest(
                "101101.011", 2, 16, 128));

        assertThat(response.result()).isEqualTo("2D.6");
        assertThat(response.exact()).isTrue();
        assertThat(response.truncated()).isFalse();
        assertThat(response.repeating()).isFalse();
        verify(engine).convert(new ConversionCommand(
                "101101.011", 2, 16, 128));
    }

    @Test
    void appliesDefaultFractionLimit() {
        when(engine.convert(new ConversionCommand(
                "0.1", 10, 2, ConversionService.DEFAULT_MAX_FRACTION_DIGITS)))
                .thenReturn(new EngineResult("0.0(0011)", true, false, true));

        ConvertResponse response = service.convert(new ConvertRequest(
                "0.1", 10, 2, null));

        assertThat(response.repeating()).isTrue();
        verify(engine).convert(new ConversionCommand(
                "0.1", 10, 2, ConversionService.DEFAULT_MAX_FRACTION_DIGITS));
    }
}
