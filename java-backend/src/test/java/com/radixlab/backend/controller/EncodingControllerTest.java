package com.radixlab.backend.controller;

import com.radixlab.backend.dto.BcdEncodeRequest;
import com.radixlab.backend.dto.BcdEncodingResponse;
import com.radixlab.backend.dto.ParityRequest;
import com.radixlab.backend.dto.ParityResponse;
import com.radixlab.backend.dto.SignedEncodeRequest;
import com.radixlab.backend.dto.SignedEncodingResponse;
import com.radixlab.backend.exception.InvalidEncodingException;
import com.radixlab.backend.exception.UnrepresentableValueException;
import com.radixlab.backend.service.EncodingService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(EncodingController.class)
class EncodingControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockitoBean
    private EncodingService encodingService;

    @Test
    void returnsSignedEncoding() throws Exception {
        when(encodingService.encodeSigned(any(SignedEncodeRequest.class)))
                .thenReturn(new SignedEncodingResponse(
                        "-13", 8, "TWOS_COMPLEMENT", "11110011", "F3",
                        true, false, "-128", "127"));

        mockMvc.perform(post("/api/v1/encoding/signed")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"value":"-13","width":8,"encoding":"TWOS_COMPLEMENT"}
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.bits").value("11110011"))
                .andExpect(jsonPath("$.hex").value("F3"))
                .andExpect(jsonPath("$.representable").value(true))
                .andExpect(jsonPath("$.minimum").value("-128"));
    }

    @Test
    void rejectsInvalidWidthBeforeServiceCall() throws Exception {
        mockMvc.perform(post("/api/v1/encoding/signed")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"value":"13","width":0,"encoding":"TWOS_COMPLEMENT"}
                                """))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value(
                        "width must be between 1 and 64"));
    }

    @Test
    void rejectsInvalidSignedBits() throws Exception {
        mockMvc.perform(post("/api/v1/encoding/signed/decode")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"bits":"1201","encoding":"SIGN_MAGNITUDE"}
                                """))
                .andExpect(status().isBadRequest());
    }

    @Test
    void mapsUnrepresentableValueTo422() throws Exception {
        when(encodingService.encodeSigned(any(SignedEncodeRequest.class)))
                .thenThrow(new UnrepresentableValueException(
                        "Value -128 is outside the representable range [-127, 127]"));

        mockMvc.perform(post("/api/v1/encoding/signed")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"value":"-128","width":8,"encoding":"SIGN_MAGNITUDE"}
                                """))
                .andExpect(status().isUnprocessableEntity())
                .andExpect(jsonPath("$.message").value(
                        "Value -128 is outside the representable range [-127, 127]"));
    }

    @Test
    void mapsIllegalBcdTo400() throws Exception {
        when(encodingService.encodeBcd(any(BcdEncodeRequest.class)))
                .thenThrow(new InvalidEncodingException("Illegal BCD nibble"));

        mockMvc.perform(post("/api/v1/encoding/bcd")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"decimal":"12"}
                                """))
                .andExpect(status().isBadRequest())
                .andExpect(jsonPath("$.message").value("Illegal BCD nibble"));
    }

    @Test
    void returnsBcdGroups() throws Exception {
        when(encodingService.encodeBcd(any(BcdEncodeRequest.class)))
                .thenReturn(new BcdEncodingResponse(
                        "59", "59", "01011001",
                        List.of("0101", "1001"), "59", "111011"));

        mockMvc.perform(post("/api/v1/encoding/bcd")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"decimal":"59"}
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.groups[0]").value("0101"))
                .andExpect(jsonPath("$.groups[1]").value("1001"))
                .andExpect(jsonPath("$.binary").value("111011"));
    }

    @Test
    void returnsParityFlipResult() throws Exception {
        when(encodingService.analyzeParity(any(ParityRequest.class)))
                .thenReturn(new ParityResponse(
                        "1010001", "1011001", 7, "EVEN",
                        4, 3, "0", "1", "10110010",
                        "10100010", false, true, 3));

        mockMvc.perform(post("/api/v1/encoding/parity")
                        .contentType(MediaType.APPLICATION_JSON)
                        .content("""
                                {"bits":"1011001","parity":"EVEN","flipBit":3}
                                """))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.valid").value(false))
                .andExpect(jsonPath("$.flippedIndex").value(3))
                .andExpect(jsonPath("$.computedParityBit").value("1"));
    }
}
