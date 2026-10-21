package com.radixlab.backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.ConfigurationPropertiesScan;

@SpringBootApplication
@ConfigurationPropertiesScan
public class RadixLabApplication {

    public static void main(String[] args) {
        SpringApplication.run(RadixLabApplication.class, args);
    }
}
