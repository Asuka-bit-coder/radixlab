package com.radixlab.backend.controller;

import com.radixlab.backend.dto.ConvertRequest;
import com.radixlab.backend.dto.ConvertResponse;
import com.radixlab.backend.exception.EngineTimeoutException;
import com.radixlab.backend.exception.InvalidNumberException;
import com.radixlab.backend.service.ConversionService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import org.springframework.test.web.servlet.MockMvc;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(ConversionController.class)
class ConversionControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockitoBean
    private ConversionService conversionService;

    @Test
    void returnsConversionResult() throws Exception {
        when(conversionService.convert(any(ConvertRequest.class)))
                .thenReturn(new ConvertResponse("2D.6", true, false, false));

        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {
                                  "number": "101101.011",
                                  "fromBase": 2,
                                  "toBase": 16
                                }
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.result").value("2D.6"))
                .andExpect(jsonPath("$.exact").value(true))
                .andExpect(jsonPath("$.truncated").value(false))
                .andExpect(jsonPath("$.repeating").value(false));
    }

    @Test
    void rejectsInvalidBaseBeforeCallingEngine() throws Exception {
        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"number":"10","fromBase":1,"toBase":10}
                                """))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value(
                        "fromBase must be between 2 and 36"));
    }

    @Test
    void rejectsMalformedNumberShape() throws Exception {
        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"number":"1.2.3","fromBase":10,"toBase":2}
                                """))
                .andExpect(status().isBadRequest());
    }

    @Test
    void rejectsMalformedJson() throws Exception {
        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("{"))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value("Malformed JSON request"));
    }

    @Test
    void mapsCliValidationErrorToBadRequest() throws Exception {
        when(conversionService.convert(any(ConvertRequest.class)))
                .thenThrow(new InvalidNumberException(
                        "Digit out of range for radix"));

        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"number":"102","fromBase":2,"toBase":10}
                                """))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value(
                        "Digit out of range for radix"));
    }

    @Test
    void mapsEngineTimeoutToGatewayTimeout() throws Exception {
        when(conversionService.convert(any(ConvertRequest.class)))
                .thenThrow(new EngineTimeoutException("timeout", null));

        mockMvc.perform(post("/api/v1/convert")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"number":"10","fromBase":2,"toBase":10}
                                """))
                .andExpect(status().isGatewayTimeout())
                .andExpect(jsonPath("$.message").value(
                        "Conversion engine timed out"));
    }
}
