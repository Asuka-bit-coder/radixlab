"use strict";

const form = document.querySelector("#convert-form");
const numberInput = document.querySelector("#number");
const fromSelect = document.querySelector("#from-base");
const toSelect = document.querySelector("#to-base");
const limitInput = document.querySelector("#fraction-limit");
const convertButton = document.querySelector("#convert-button");
const buttonLabel = convertButton.querySelector(".button-label");
const formError = document.querySelector("#form-error");
const resultEmpty = document.querySelector("#result-empty");
const resultContent = document.querySelector("#result-content");
const resultExpression = document.querySelector("#result-expression");
const resultOutput = document.querySelector("#result-output");
const resultStatus = document.querySelector("#result-status");
const resultDetail = document.querySelector("#result-detail");
const copyButton = document.querySelector("#copy-result");
const swapButton = document.querySelector("#swap-bases");

const BASE_NAMES = {
    2: "Binary",
    3: "Ternary",
    4: "Quaternary",
    5: "Quinary",
    6: "Senary",
    7: "Septenary",
    8: "Octal",
    9: "Nonary",
    10: "Decimal",
    11: "Undecimal",
    12: "Duodecimal",
    13: "Tridecimal",
    14: "Tetradecimal",
    15: "Pentadecimal",
    16: "Hexadecimal",
    20: "Vigesimal",
    36: "Hexatrigesimal"
};

let lastResult = "";

function populateBaseSelect(select, selected) {
    const fragment = document.createDocumentFragment();
    for (let base = 2; base <= 36; base += 1) {
        const option = document.createElement("option");
        option.value = String(base);
        option.textContent = BASE_NAMES[base]
            ? `${base} · ${BASE_NAMES[base]}`
            : `${base}`;
        option.selected = base === selected;
        fragment.append(option);
    }
    select.append(fragment);
}

populateBaseSelect(fromSelect, 2);
populateBaseSelect(toSelect, 16);

function showError(message) {
    formError.textContent = message;
    formError.hidden = false;
    numberInput.setAttribute("aria-invalid", "true");
}

function clearError() {
    formError.textContent = "";
    formError.hidden = true;
    numberInput.removeAttribute("aria-invalid");
}

function setBusy(busy) {
    convertButton.disabled = busy;
    buttonLabel.textContent = busy ? "Converting" : "Convert";
    form.setAttribute("aria-busy", String(busy));
}

function digitValue(character) {
    const code = character.codePointAt(0);
    if (code >= 48 && code <= 57) {
        return code - 48;
    }
    if (code >= 65 && code <= 90) {
        return code - 65 + 10;
    }
    if (code >= 97 && code <= 122) {
        return code - 97 + 10;
    }
    return -1;
}

function validateForm() {
    const numeral = numberInput.value.trim();
    const fromBase = Number(fromSelect.value);
    const toBase = Number(toSelect.value);
    const limit = Number(limitInput.value);

    if (numeral.length === 0) {
        return { error: "Enter a numeral to convert." };
    }

    if (numeral.length > 4096) {
        return { error: "The numeral must contain at most 4096 characters." };
    }

    if (!Number.isInteger(limit) || limit < 0 || limit > 4096) {
        return { error: "The fraction digit limit must be between 0 and 4096." };
    }

    const unsigned = numeral.replace(/^[+-]/, "");
    if (unsigned.length === 0) {
        return { error: "A sign must be followed by at least one digit." };
    }

    const parts = unsigned.split(".");
    if (parts.length > 2) {
        return { error: "Use at most one radix point." };
    }

    let digitCount = 0;
    for (const part of parts) {
        for (const character of part) {
            const value = digitValue(character);
            if (value < 0) {
                return {
                    error: `“${character}” is not a digit. Use 0–9 and A–Z.`
                };
            }
            if (value >= fromBase) {
                return {
                    error: `“${character}” is not valid in base ${fromBase}.`
                };
            }
            digitCount += 1;
        }
    }

    if (digitCount === 0) {
        return { error: "The numeral must contain at least one digit." };
    }

    return { numeral, fromBase, toBase, limit };
}

function pluralize(count, singular) {
    return `${count} ${singular}${count === 1 ? "" : "s"}`;
}

function renderResult(data, request) {
    lastResult = data.result;
    resultEmpty.hidden = true;
    resultContent.hidden = false;
    copyButton.hidden = false;

    resultExpression.textContent =
        `${request.numeral} in base ${request.fromBase} → base ${request.toBase}`;
    resultOutput.textContent = data.result;

    resultStatus.replaceChildren();
    const badge = document.createElement("span");
    badge.className = "status-badge";

    let detail;
    if (data.truncated) {
        badge.classList.add("is-warning");
        badge.textContent = `Truncated at ${request.limit} fraction digits`;
        detail = "The cycle did not close within the chosen limit. Increase the limit for a longer prefix or the complete cycle.";
    } else if (data.repeating) {
        badge.textContent = "Exact · repeating cycle";
        detail = "The parenthesized digits repeat indefinitely. This is the complete value, not an approximation.";
    } else if (data.exact && data.result.includes(".")) {
        badge.textContent = "Exact · terminating";
        detail = "This fraction terminates in the target base.";
    } else {
        badge.textContent = "Exact · integer";
        detail = "The source value has no fractional component in the target base.";
    }

    resultStatus.append(badge);
    resultDetail.textContent = detail;
}

function clearResult() {
    lastResult = "";
    copyButton.hidden = true;
    copyButton.textContent = "Copy";
    resultContent.hidden = true;
    resultEmpty.hidden = false;
}

async function convert(event) {
    event.preventDefault();
    clearError();

    const validation = validateForm();
    if (validation.error) {
        showError(validation.error);
        numberInput.focus();
        return;
    }

    setBusy(true);
    const controller = new AbortController();
    const timeout = window.setTimeout(() => controller.abort(), 12000);

    try {
        const response = await fetch("/api/v1/convert", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                number: validation.numeral,
                fromBase: validation.fromBase,
                toBase: validation.toBase,
                maxFractionDigits: validation.limit
            }),
            signal: controller.signal
        });

        let body;
        try {
            body = await response.json();
        } catch {
            throw new Error("The server returned an unreadable response.");
        }

        if (!response.ok) {
            throw new Error(body.message || "The conversion could not be completed.");
        }

        renderResult(body, validation);
    } catch (error) {
        clearResult();
        if (error.name === "AbortError") {
            showError("The conversion took too long. Try a shorter input or a lower digit limit.");
        } else {
            showError(error.message || "The conversion service is unavailable.");
        }
    } finally {
        window.clearTimeout(timeout);
        setBusy(false);
    }
}

form.addEventListener("submit", convert);

numberInput.addEventListener("input", () => {
    clearError();
});

swapButton.addEventListener("click", () => {
    const from = fromSelect.value;
    fromSelect.value = toSelect.value;
    toSelect.value = from;
});

document.querySelectorAll(".example").forEach((button) => {
    button.addEventListener("click", () => {
        numberInput.value = button.dataset.number;
        fromSelect.value = button.dataset.from;
        toSelect.value = button.dataset.to;
        limitInput.value = "256";
        clearError();
        form.requestSubmit();
    });
});

copyButton.addEventListener("click", async () => {
    if (!lastResult) {
        return;
    }

    try {
        await navigator.clipboard.writeText(lastResult);
        copyButton.textContent = "Copied";
        window.setTimeout(() => {
            copyButton.textContent = "Copy";
        }, 1600);
    } catch {
        copyButton.textContent = "Select result";
        resultOutput.focus?.();
    }
});
