if(NOT DEFINED CLI)
    message(FATAL_ERROR "CLI executable path was not provided")
endif()

set(passed 0)

function(expect_cli name expected_exit expected_stdout)
    execute_process(
        COMMAND "${CLI}" ${ARGN}
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        TIMEOUT 10
    )

    string(STRIP "${stdout}" stdout)
    string(STRIP "${stderr}" stderr)

    if(NOT exit_code EQUAL expected_exit)
        message(FATAL_ERROR
            "${name}: expected exit ${expected_exit}, got ${exit_code}\n"
            "stdout: ${stdout}\nstderr: ${stderr}"
        )
    endif()

    if(NOT stdout STREQUAL expected_stdout)
        message(FATAL_ERROR
            "${name}: expected stdout '${expected_stdout}', got '${stdout}'\n"
            "stderr: ${stderr}"
        )
    endif()

    message(STATUS "[PASS] ${name}")
    math(EXPR next "${passed} + 1")
    set(passed ${next} PARENT_SCOPE)
endfunction()

function(expect_stdout_contains name expected_exit expected_fragment)
    execute_process(
        COMMAND "${CLI}" ${ARGN}
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        TIMEOUT 10
    )

    if(NOT exit_code EQUAL expected_exit)
        message(FATAL_ERROR
            "${name}: expected exit ${expected_exit}, got ${exit_code}\n"
            "stdout: ${stdout}\nstderr: ${stderr}"
        )
    endif()

    string(FIND "${stdout}" "${expected_fragment}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR
            "${name}: stdout does not contain '${expected_fragment}'\n"
            "stdout: ${stdout}\nstderr: ${stderr}"
        )
    endif()

    message(STATUS "[PASS] ${name}")
    math(EXPR next "${passed} + 1")
    set(passed ${next} PARENT_SCOPE)
endfunction()

function(expect_cli_error name expected_exit expected_stderr_fragment)
    execute_process(
        COMMAND "${CLI}" ${ARGN}
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        TIMEOUT 10
    )

    if(NOT exit_code EQUAL expected_exit)
        message(FATAL_ERROR
            "${name}: expected exit ${expected_exit}, got ${exit_code}\n"
            "stdout: ${stdout}\nstderr: ${stderr}"
        )
    endif()

    if(NOT stdout STREQUAL "")
        message(FATAL_ERROR
            "${name}: expected empty stdout, got '${stdout}'"
        )
    endif()

    string(FIND "${stderr}" "${expected_stderr_fragment}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR
            "${name}: stderr does not contain '${expected_stderr_fragment}'\n"
            "stderr: ${stderr}"
        )
    endif()

    message(STATUS "[PASS] ${name}")
    math(EXPR next "${passed} + 1")
    set(passed ${next} PARENT_SCOPE)
endfunction()

expect_cli(
    "version comes from the build"
    0 "radixlab-cli 0.2.0"
    --version
)

expect_cli(
    "documented fraction conversion"
    0 "2D.6"
    --input 101101.011 --from 2 --to 16
)

expect_cli(
    "repeating fraction"
    0 "0.0(0011)"
    --input 0.1 --from 10 --to 2
)

expect_cli(
    "negative integer"
    0 "-FF"
    --input -255 --from 10 --to 16
)

expect_cli(
    "base 36 output"
    0 "Z"
    --input 35 --from 10 --to 36
)

expect_cli(
    "truncated output"
    0 "0.00"
    --input 0.1 --from 10 --to 2 --max-fraction-digits 2
)

expect_stdout_contains(
    "structured JSON success"
    0 "\"result\":\"0.0(0011)\""
    --input 0.1 --from 10 --to 2 --json
)

expect_stdout_contains(
    "structured JSON truncation flag"
    0 "\"truncated\":true"
    --input 0.1 --from 10 --to 2 --json --max-fraction-digits 1
)

expect_cli_error(
    "invalid numeral"
    2 "Digit out of range"
    --input 102 --from 2 --to 10
)

expect_cli_error(
    "missing option"
    2 "Missing required option: --to"
    --input 10 --from 2
)

expect_cli_error(
    "invalid base"
    2 "must be between 2 and 36"
    --input 10 --from 1 --to 10
)

expect_cli_error(
    "JSON error is machine-readable"
    2 "\"error\":{"
    --input 2 --from 2 --to 10 --json
)

expect_cli_error(
    "JSON usage error is machine-readable"
    2 "\"error\":{"
    --json --input 10 --from 2
)

string(REPEAT "1" 4097 oversized_input)
expect_cli_error(
    "input length limit"
    3 "4096-character limit"
    --input "${oversized_input}" --from 2 --to 10
)

expect_stdout_contains(
    "signed encoding JSON"
    0 "\"bits\":\"11110011\""
    encoding --operation signed-encode --value -13 --width 8
    --encoding TWOS_COMPLEMENT --json
)

expect_stdout_contains(
    "signed negative-zero decode"
    0 "\"negativeZero\":true"
    encoding --operation signed-decode --bits 10000000
    --encoding SIGN_MAGNITUDE --json
)

expect_cli_error(
    "signed range error exit code"
    4 "outside the representable range"
    encoding --operation signed-encode --value -128 --width 8
    --encoding SIGN_MAGNITUDE --json
)

expect_stdout_contains(
    "BCD groups"
    0 "\"groups\":[\"0101\",\"1001\"]"
    encoding --operation bcd-encode --decimal 59 --json
)

expect_cli_error(
    "illegal BCD nibble"
    2 "Illegal BCD nibble"
    encoding --operation bcd-decode --bits 1010 --json
)

expect_stdout_contains(
    "Gray encoding"
    0 "\"gray\":\"00001011\""
    encoding --operation gray-encode --value 13 --width 8 --json
)

expect_stdout_contains(
    "BCD decoding"
    0 "\"digits\":\"59\""
    encoding --operation bcd-decode --bits 01011001 --json
)

string(REPEAT "0" 4100 oversized_bcd)
expect_cli_error(
    "BCD bit limit"
    3 "4096-bit BCD limit"
    encoding --operation bcd-decode --bits "${oversized_bcd}" --json
)

expect_stdout_contains(
    "Gray decoding"
    0 "\"value\":\"13\""
    encoding --operation gray-decode --bits 00001011 --json
)

expect_stdout_contains(
    "parity single-bit flip detected"
    0 "\"valid\":false"
    encoding --operation parity --bits 1011001 --parity EVEN
    --flip-bit 3 --json
)

expect_stdout_contains(
    "encoding help"
    0 "Digital logic encodings"
    encoding --help
)

message(STATUS "CLI checks passed: ${passed}")
