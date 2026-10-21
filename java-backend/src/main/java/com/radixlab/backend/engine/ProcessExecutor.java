package com.radixlab.backend.engine;

import java.time.Duration;
import java.util.List;

/**
 * Boundary around process creation so engine tests can use a deterministic fake.
 */
public interface ProcessExecutor {

    ProcessOutput execute(List<String> command, Duration timeout);
}
