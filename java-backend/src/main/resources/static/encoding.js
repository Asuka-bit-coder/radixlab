"use strict";

const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];

function setStatus(element, text, kind = "") {
    element.textContent = text;
    element.className = kind ? `lab-status is-${kind}` : "lab-status";
}

function showError(element, message) {
    element.textContent = message;
    element.hidden = false;
}

function clearError(element) {
    element.textContent = "";
    element.hidden = true;
}

function setBusy(form, busy) {
    const button = form.querySelector('button[type="submit"]');
    if (button) {
        button.disabled = busy;
    }
    form.setAttribute("aria-busy", String(busy));
}

function debounce(action, delay = 320) {
    let timer = 0;
    return () => {
        window.clearTimeout(timer);
        timer = window.setTimeout(action, delay);
    };
}

async function postJson(path, body) {
    const controller = new AbortController();
    const timeout = window.setTimeout(() => controller.abort(), 12000);

    try {
        const response = await fetch(path, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body),
            signal: controller.signal
        });

        let payload = null;
        try {
            payload = await response.json();
        } catch {
            throw new Error("The server returned an unreadable response.");
        }

        if (!response.ok) {
            throw new Error(payload.message || `Request failed (${response.status}).`);
        }

        return payload;
    } catch (error) {
        if (error.name === "AbortError") {
            throw new Error("The encoding engine took too long to respond.");
        }
        throw error;
    } finally {
        window.clearTimeout(timeout);
    }
}

function createBitCell(bit, index, extraClass = "", indexText = String(index)) {
    const cell = document.createElement("span");
    cell.className = "bit-cell";
    if (extraClass) {
        cell.classList.add(extraClass);
    }
    cell.title = index >= 0 ? `b${index}: ${bit}` : `${indexText}: ${bit}`;

    const value = document.createElement("span");
    value.className = "bit-value";
    value.textContent = bit;

    const label = document.createElement("span");
    label.className = "bit-index";
    label.textContent = indexText;

    cell.append(value, label);
    return cell;
}

function renderBitGrid(container, bits, options = {}) {
    const highlight = options.highlight || new Set();
    const sign = Boolean(options.sign);
    const width = bits.length;

    container.replaceChildren();
    container.setAttribute(
        "aria-label",
        options.label || `Bit pattern ${bits}, most significant bit first`
    );

    for (let position = 0; position < width; position += 1) {
        const index = width - 1 - position;
        const classes = [
            sign && position === 0 ? "is-sign" : "",
            highlight.has(index) ? "is-changed" : ""
        ].filter(Boolean).join(" ");
        container.append(
            createBitCell(bits[position], index, classes)
        );
    }
}

function digitsOnly(value) {
    return /^[0-9]+$/.test(value);
}

function bitsOnly(value, maxLength = 64) {
    return new RegExp(`^[01]{1,${maxLength}}$`).test(value);
}

function valueLength(value, maximum) {
    return value.length <= maximum;
}

/* -------------------------------------------------------------------------
 * Tabs
 * ---------------------------------------------------------------------- */

const tabs = $$(".lab-tab");
const panelInitializers = new Map();
const initializedPanels = new Set();

function activateTab(tab, options = {}) {
    if (!tab) {
        return;
    }

    tabs.forEach((candidate) => {
        const selected = candidate === tab;
        candidate.setAttribute("aria-selected", String(selected));
        candidate.tabIndex = selected ? 0 : -1;

        const panel = document.getElementById(candidate.dataset.panel);
        if (panel) {
            panel.hidden = !selected;
        }
    });

    if (options.focus) {
        tab.focus();
    }

    const key = tab.dataset.panel.replace("panel-", "");
    if (options.updateHash !== false) {
        window.history.replaceState(null, "", `#${key}`);
    }

    if (!initializedPanels.has(key) && panelInitializers.has(key)) {
        initializedPanels.add(key);
        panelInitializers.get(key)();
    }
}

tabs.forEach((tab, position) => {
    tab.addEventListener("click", () => activateTab(tab, { focus: false }));

    tab.addEventListener("keydown", (event) => {
        let next = null;
        if (event.key === "ArrowRight") {
            next = tabs[(position + 1) % tabs.length];
        } else if (event.key === "ArrowLeft") {
            next = tabs[(position - 1 + tabs.length) % tabs.length];
        } else if (event.key === "Home") {
            next = tabs[0];
        } else if (event.key === "End") {
            next = tabs[tabs.length - 1];
        }

        if (next) {
            event.preventDefault();
            activateTab(next, { focus: true });
        }
    });
});

/* -------------------------------------------------------------------------
 * Signed explorer
 * ---------------------------------------------------------------------- */

const signedForm = $("#signed-form");
const signedValue = $("#signed-value");
const signedWidth = $("#signed-width");
const signedEncoding = $("#signed-encoding");
const signedNegativeZero = $("#signed-negative-zero");
const signedNegativeZeroRow = $("#signed-negative-zero-row");
const signedEncodeFields = $("#signed-encode-fields");
const signedDecodeFields = $("#signed-decode-fields");
const signedBits = $("#signed-bits");
const signedDecodeEncoding = $("#signed-decode-encoding");
const signedError = $("#signed-error");
const signedStatus = $("#signed-status");
const signedBitGrid = $("#signed-bit-grid");
const signedWidthLabel = $("#signed-width-label");
const signedValueOutput = $("#signed-value-output");
const signedHexOutput = $("#signed-hex-output");
const signedRangeOutput = $("#signed-range-output");
const signedNote = $("#signed-note");

let signedRequest = 0;
const scheduleSigned = debounce(() => refreshSigned());

function signedDirection() {
    return $('input[name="signed-direction"]:checked').value;
}

function syncSignedForm() {
    const decode = signedDirection() === "decode";
    signedEncodeFields.hidden = decode;
    signedDecodeFields.hidden = !decode;

    const twoComplement = signedEncoding.value === "TWOS_COMPLEMENT";
    signedNegativeZero.disabled = twoComplement;
    signedNegativeZeroRow.classList.toggle("is-disabled", twoComplement);
    if (twoComplement) {
        signedNegativeZero.checked = false;
    }
}

function clearSignedDisplay() {
    signedBitGrid.replaceChildren();
    signedValueOutput.textContent = "—";
    signedHexOutput.textContent = "—";
    signedRangeOutput.textContent = "—";
    signedNote.hidden = true;
}

function renderSigned(data) {
    const width = data.width;
    signedWidthLabel.textContent = `${width}-bit word`;
    renderBitGrid(signedBitGrid, data.bits, {
        sign: true,
        label: `${width}-bit signed pattern ${data.bits}, most significant sign bit first`
    });

    signedValueOutput.textContent = data.negativeZero
        ? `${data.value} (negative zero)`
        : data.value;
    signedHexOutput.textContent = `0x${data.hex}`;
    signedRangeOutput.textContent = `${data.minimum} … ${data.maximum}`;

    if (data.negativeZero) {
        signedNote.textContent =
            "This code pattern has numeric value zero but is distinct from the positive-zero pattern. It is not available in two's complement.";
        signedNote.hidden = false;
        setStatus(signedStatus, "Negative zero pattern", "warning");
    } else {
        signedNote.hidden = true;
        setStatus(signedStatus, "Representable", "ok");
    }
}

async function refreshSigned() {
    const request = ++signedRequest;
    clearError(signedError);
    const decode = signedDirection() === "decode";

    let body;
    let path;

    if (decode) {
        const bits = signedBits.value.trim();
        if (!bitsOnly(bits)) {
            setBusy(signedForm, false);
            clearSignedDisplay();
            setStatus(signedStatus, "Invalid input", "error");
            showError(signedError, "Enter 1–64 binary digits.");
            return;
        }
        path = "/api/v1/encoding/signed/decode";
        body = { bits, encoding: signedDecodeEncoding.value };
    } else {
        const value = signedValue.value.trim();
        if (!/^[+-]?[0-9]+$/.test(value) || !valueLength(value, 64)) {
            setBusy(signedForm, false);
            clearSignedDisplay();
            setStatus(signedStatus, "Invalid input", "error");
            showError(signedError, "Enter a decimal integer of at most 64 characters.");
            return;
        }
        path = "/api/v1/encoding/signed";
        body = {
            value,
            width: Number(signedWidth.value),
            encoding: signedEncoding.value,
            negativeZero: signedNegativeZero.checked
        };
    }

    setBusy(signedForm, true);
    setStatus(signedStatus, "Computing…");

    try {
        const data = await postJson(path, body);
        if (request !== signedRequest) {
            return;
        }
        renderSigned(data);
    } catch (error) {
        if (request !== signedRequest) {
            return;
        }
        clearSignedDisplay();
        setStatus(signedStatus, "Error", "error");
        showError(signedError, error.message);
    } finally {
        if (request === signedRequest) {
            setBusy(signedForm, false);
        }
    }
}

$$('input[name="signed-direction"]').forEach((radio) => {
    radio.addEventListener("change", () => {
        syncSignedForm();
        refreshSigned();
    });
});

signedValue.addEventListener("input", scheduleSigned);
signedWidth.addEventListener("change", scheduleSigned);
signedEncoding.addEventListener("change", () => {
    syncSignedForm();
    scheduleSigned();
});
signedNegativeZero.addEventListener("change", scheduleSigned);
signedBits.addEventListener("input", scheduleSigned);
signedDecodeEncoding.addEventListener("change", scheduleSigned);

signedForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshSigned();
});

syncSignedForm();
panelInitializers.set("signed", refreshSigned);

/* -------------------------------------------------------------------------
 * BCD explorer
 * ---------------------------------------------------------------------- */

const bcdForm = $("#bcd-form");
const bcdDecimal = $("#bcd-decimal");
const bcdError = $("#bcd-error");
const bcdStatus = $("#bcd-status");
const bcdGroups = $("#bcd-groups");
const bcdBitsOutput = $("#bcd-bits-output");
const bcdBinaryOutput = $("#bcd-binary-output");
const bcdValueOutput = $("#bcd-value-output");

let bcdRequest = 0;
const scheduleBcd = debounce(() => refreshBcd());

function clearBcdDisplay() {
    bcdGroups.replaceChildren();
    bcdBitsOutput.textContent = "—";
    bcdBinaryOutput.textContent = "—";
    bcdValueOutput.textContent = "—";
}

function renderBcd(data) {
    bcdGroups.replaceChildren();

    data.groups.forEach((nibble, index) => {
        const card = document.createElement("div");
        card.className = "bcd-card";

        const digit = document.createElement("span");
        digit.className = "bcd-digit";
        digit.textContent = data.digits[index] ?? "?";

        const bits = document.createElement("code");
        bits.className = "bcd-nibble";
        bits.textContent = nibble;

        card.append(digit, bits);
        bcdGroups.append(card);
    });

    bcdBitsOutput.textContent = data.groups.join(" ");
    bcdBinaryOutput.textContent = data.binary;
    bcdValueOutput.textContent = data.value;
    setStatus(
        bcdStatus,
        `${data.groups.length} group${data.groups.length === 1 ? "" : "s"}`,
        "ok"
    );
}

async function refreshBcd() {
    const request = ++bcdRequest;
    clearError(bcdError);

    const decimal = bcdDecimal.value.trim();
    if (!digitsOnly(decimal) || !valueLength(decimal, 1024)) {
        setBusy(bcdForm, false);
        clearBcdDisplay();
        setStatus(bcdStatus, "Invalid input", "error");
        showError(bcdError, "Enter 1–1024 decimal digits (0–9 only).");
        return;
    }

    setBusy(bcdForm, true);
    setStatus(bcdStatus, "Computing…");

    try {
        const data = await postJson("/api/v1/encoding/bcd", { decimal });
        if (request !== bcdRequest) {
            return;
        }
        renderBcd(data);
    } catch (error) {
        if (request !== bcdRequest) {
            return;
        }
        clearBcdDisplay();
        setStatus(bcdStatus, "Error", "error");
        showError(bcdError, error.message);
    } finally {
        if (request === bcdRequest) {
            setBusy(bcdForm, false);
        }
    }
}

bcdDecimal.addEventListener("input", scheduleBcd);
bcdForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshBcd();
});

panelInitializers.set("bcd", refreshBcd);

/* -------------------------------------------------------------------------
 * Gray code explorer
 * ---------------------------------------------------------------------- */

const grayForm = $("#gray-form");
const grayValue = $("#gray-value");
const grayWidth = $("#gray-width");
const grayPrev = $("#gray-prev");
const grayNext = $("#gray-next");
const grayError = $("#gray-error");
const grayStatus = $("#gray-status");
const grayValueOutput = $("#gray-value-output");
const grayBinaryGrid = $("#gray-binary-grid");
const grayGrayGrid = $("#gray-gray-grid");
const grayTransition = $("#gray-transition");
const grayChanged = $("#gray-changed");
const grayPrevValue = $("#gray-prev-value");
const grayCurrentValue = $("#gray-current-value");
const grayTransitionGrid = $("#gray-transition-grid");

let grayRequest = 0;
const scheduleGray = debounce(() => refreshGray());

function clearGrayDisplay() {
    grayBinaryGrid.replaceChildren();
    grayGrayGrid.replaceChildren();
    grayTransition.hidden = true;
    grayValueOutput.textContent = "—";
}

function updateGraySteppers(numericValue, width) {
    const maximum = (1n << BigInt(width)) - 1n;
    grayPrev.disabled = numericValue <= 0n;
    grayNext.disabled = numericValue >= maximum;
}

function disableGraySteppers() {
    grayPrev.disabled = true;
    grayNext.disabled = true;
}

function renderGray(current, previous) {
    grayValueOutput.textContent = current.value;
    renderBitGrid(grayBinaryGrid, current.binary, {
        label: `Binary pattern ${current.binary} for value ${current.value}`
    });
    renderBitGrid(grayGrayGrid, current.gray, {
        label: `Gray code ${current.gray} for value ${current.value}`
    });

    if (!previous) {
        grayTransition.hidden = true;
    } else {
        const changed = [];
        for (let index = 0; index < current.gray.length; index += 1) {
            if (previous.gray[index] !== current.gray[index]) {
                const bitIndex = current.gray.length - 1 - index;
                changed.push(bitIndex);
            }
        }

        grayChanged.textContent = changed.length === 1
            ? `bit b${changed[0]} changed`
            : `${changed.length} bits changed`;
        grayPrevValue.textContent = `gray ${previous.gray} = ${previous.value}`;
        grayCurrentValue.textContent = `gray ${current.gray} = ${current.value}`;
        renderBitGrid(grayTransitionGrid, current.gray, {
            highlight: new Set(changed),
            label: `Current Gray code ${current.gray}; changed bit ${changed.join(", ")}`
        });
        grayTransition.hidden = false;
    }

    setStatus(grayStatus, "Gray code ready", "ok");
}

async function refreshGray() {
    const request = ++grayRequest;
    clearError(grayError);

    const text = grayValue.value.trim();
    const width = Number(grayWidth.value);

    if (!digitsOnly(text) || !valueLength(text, 64)) {
        setBusy(grayForm, false);
        clearGrayDisplay();
        disableGraySteppers();
        setStatus(grayStatus, "Invalid input", "error");
        showError(grayError, "Enter an unsigned decimal integer of at most 64 digits.");
        return;
    }

    let numericValue;
    try {
        numericValue = BigInt(text);
    } catch {
        setBusy(grayForm, false);
        clearGrayDisplay();
        disableGraySteppers();
        setStatus(grayStatus, "Invalid input", "error");
        showError(grayError, "The value is not a valid integer.");
        return;
    }

    const maximum = (1n << BigInt(width)) - 1n;
    if (numericValue > maximum) {
        setBusy(grayForm, false);
        clearGrayDisplay();
        disableGraySteppers();
        setStatus(grayStatus, "Out of range", "error");
        showError(grayError, `Value ${text} does not fit in ${width} bits (maximum ${maximum}).`);
        return;
    }

    setBusy(grayForm, true);
    setStatus(grayStatus, "Computing…");

    try {
        const current = await postJson("/api/v1/encoding/gray", {
            value: text,
            width
        });

        let previous = null;
        if (numericValue > 0n) {
            previous = await postJson("/api/v1/encoding/gray", {
                value: (numericValue - 1n).toString(),
                width
            });
        }

        if (request !== grayRequest) {
            return;
        }

        renderGray(current, previous);
        updateGraySteppers(BigInt(current.value), width);
    } catch (error) {
        if (request !== grayRequest) {
            return;
        }
        clearGrayDisplay();
        disableGraySteppers();
        setStatus(grayStatus, "Error", "error");
        showError(grayError, error.message);
    } finally {
        if (request === grayRequest) {
            setBusy(grayForm, false);
        }
    }
}

function stepGray(delta) {
    const text = grayValue.value.trim();
    if (!digitsOnly(text)) {
        return;
    }

    const current = BigInt(text);
    const width = Number(grayWidth.value);
    const maximum = (1n << BigInt(width)) - 1n;
    const next = current + BigInt(delta);

    if (next < 0n || next > maximum) {
        return;
    }

    grayValue.value = next.toString();
    refreshGray();
}

grayPrev.addEventListener("click", () => stepGray(-1));
grayNext.addEventListener("click", () => stepGray(1));
grayValue.addEventListener("input", scheduleGray);
grayWidth.addEventListener("change", scheduleGray);
grayForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshGray();
});

disableGraySteppers();
panelInitializers.set("gray", refreshGray);

/* -------------------------------------------------------------------------
 * Parity explorer
 * ---------------------------------------------------------------------- */

const parityForm = $("#parity-form");
const parityBits = $("#parity-bits");
const parityMode = $("#parity-mode");
const parityFlip = $("#parity-flip");
const parityError = $("#parity-error");
const parityStatus = $("#parity-status");
const parityWord = $("#parity-word");
const parityOnesOutput = $("#parity-ones-output");
const parityExpectedOutput = $("#parity-expected-output");
const parityComputedOutput = $("#parity-computed-output");
const parityMessage = $("#parity-message");

let parityRequest = 0;
const scheduleParity = debounce(() => refreshParity());

function syncParityFlipOptions() {
    const bits = parityBits.value.trim();
    const width = bits.length;
    const previous = parityFlip.value;

    parityFlip.replaceChildren();
    const none = document.createElement("option");
    none.value = "";
    none.textContent = "No flip";
    parityFlip.append(none);

    if (bitsOnly(bits) && width > 0) {
        for (let index = width - 1; index >= 0; index -= 1) {
            const option = document.createElement("option");
            option.value = String(index);
            option.textContent = index === 0
                ? `Flip bit 0 (LSB)`
                : `Flip bit ${index}`;
            parityFlip.append(option);
        }
    }

    if (previous !== "" && Number(previous) < width) {
        parityFlip.value = previous;
    }
}

function clearParityDisplay() {
    parityWord.replaceChildren();
    parityOnesOutput.textContent = "—";
    parityExpectedOutput.textContent = "—";
    parityComputedOutput.textContent = "—";
    parityMessage.textContent = "";
}

function renderParity(data) {
    parityWord.replaceChildren();

    const flippedIndex = data.flippedIndex;
    for (let index = data.bits.length - 1; index >= 0; index -= 1) {
        const position = data.bits.length - 1 - index;
        const extra = data.flipped && index === flippedIndex
            ? "is-flipped"
            : "";
        parityWord.append(
            createBitCell(data.bits[position], index, extra)
        );
    }

    const separator = document.createElement("span");
    separator.className = "parity-separator";
    separator.textContent = "|";
    separator.setAttribute("aria-hidden", "true");

    const parityCell = createBitCell(
        data.parityBit,
        -1,
        "is-parity",
        "P"
    );
    parityWord.append(separator, parityCell);
    parityWord.setAttribute(
        "aria-label",
        `Received data ${data.bits} with transmitted parity bit ${data.parityBit}`
    );

    parityOnesOutput.textContent = data.flipped
        ? `${data.onesCount} → ${data.receivedOnesCount}`
        : String(data.onesCount);
    parityExpectedOutput.textContent = data.parityBit;
    parityComputedOutput.textContent = data.computedParityBit;

    if (data.valid) {
        setStatus(parityStatus, "Check passed", "ok");
        parityMessage.classList.add("is-neutral");
        parityMessage.textContent =
            `The receiver recomputed parity ${data.computedParityBit}, matching the transmitted bit.`;
    } else {
        setStatus(parityStatus, "Error detected", "warning");
        parityMessage.classList.remove("is-neutral");
        parityMessage.textContent =
            `One data bit changed: original ${data.originalBits}, received ${data.bits}. ` +
            `The receiver recomputed ${data.computedParityBit} but received ${data.parityBit}. ` +
            "Parity flags the error but cannot identify or repair the changed bit.";
    }
}

async function refreshParity() {
    const request = ++parityRequest;
    clearError(parityError);

    const bits = parityBits.value.trim();
    if (!bitsOnly(bits)) {
        setBusy(parityForm, false);
        clearParityDisplay();
        setStatus(parityStatus, "Invalid input", "error");
        showError(parityError, "Enter 1–64 binary digits.");
        return;
    }

    const body = {
        bits,
        parity: parityMode.value
    };

    if (parityFlip.value !== "") {
        body.flipBit = Number(parityFlip.value);
        if (body.flipBit >= bits.length) {
            setBusy(parityForm, false);
            clearParityDisplay();
            setStatus(parityStatus, "Invalid input", "error");
            showError(parityError, "The selected flip bit is outside the data width.");
            return;
        }
    }

    setBusy(parityForm, true);
    setStatus(parityStatus, "Computing…");

    try {
        const data = await postJson("/api/v1/encoding/parity", body);
        if (request !== parityRequest) {
            return;
        }
        renderParity(data);
    } catch (error) {
        if (request !== parityRequest) {
            return;
        }
        clearParityDisplay();
        setStatus(parityStatus, "Error", "error");
        showError(parityError, error.message);
    } finally {
        if (request === parityRequest) {
            setBusy(parityForm, false);
        }
    }
}

parityBits.addEventListener("input", () => {
    syncParityFlipOptions();
    scheduleParity();
});
parityMode.addEventListener("change", scheduleParity);
parityFlip.addEventListener("change", refreshParity);
parityForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshParity();
});

syncParityFlipOptions();
panelInitializers.set("parity", refreshParity);

/* -------------------------------------------------------------------------
 * Initial tab
 * ---------------------------------------------------------------------- */

const requestedKey = window.location.hash.replace("#", "");
const requestedTab = $(`#tab-${requestedKey}`);
activateTab(requestedTab || tabs[0], { focus: false, updateHash: false });
