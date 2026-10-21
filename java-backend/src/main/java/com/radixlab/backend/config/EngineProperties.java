package com.radixlab.backend.config;

import java.time.Duration;

import org.springframework.boot.context.properties.ConfigurationProperties;

@ConfigurationProperties(prefix = "radixlab.engine")
public class EngineProperties {

    private String cliPath = "radixlab-cli";
    private Duration timeout = Duration.ofSeconds(5);
    private int maxConcurrent = 4;

    public String getCliPath() {
        return cliPath;
    }

    public void setCliPath(String cliPath) {
        this.cliPath = cliPath;
    }

    public Duration getTimeout() {
        return timeout;
    }

    public void setTimeout(Duration timeout) {
        this.timeout = timeout;
    }

    public int getMaxConcurrent() {
        return maxConcurrent;
    }

    public void setMaxConcurrent(int maxConcurrent) {
        this.maxConcurrent = maxConcurrent;
    }
}
