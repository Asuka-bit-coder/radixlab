"use strict";

const $ = (selector, root = document) => root.querySelector(selector);

class BooleanApiError extends Error {
    constructor(message, status, code = "", position = null, column = null) {
        super(message);
        this.name = "BooleanApiError";
        this.status = status;
        this.code = code;
        this.position = Number.isInteger(position) ? position : null;
        this.column = Number.isInteger(column) ? column : null;
    }
}

async function fetchBoolean(path, body) {
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
            throw new BooleanApiError(
                "The server returned an unreadable response.", response.status);
        }

        if (!response.ok) {
            throw new BooleanApiError(
                payload.message || `Request failed (${response.status}).`,
                response.status,
                payload.code || "",
                payload.position,
                payload.column
            );
        }

        return payload;
    } catch (error) {
        if (error instanceof BooleanApiError) {
            throw error;
        }
        if (error?.name === "AbortError") {
            throw new BooleanApiError(
                "The Boolean engine took too long to respond.", 0);
        }
        throw new BooleanApiError(
            "Could not reach the Boolean engine. Check that the server is running.",
            0
        );
    } finally {
        window.clearTimeout(timeout);
    }
}

function setStatus(element, text, kind = "") {
    element.textContent = text;
    element.className = kind ? `lab-status is-${kind}` : "lab-status";
}

function debounce(action, delay = 320) {
    let timer = 0;
    return () => {
        window.clearTimeout(timer);
        timer = window.setTimeout(action, delay);
    };
}

function setBusy(form, busy) {
    const button = form.querySelector('button[type="submit"]');
    if (button) {
        button.disabled = busy;
    }
    form.setAttribute("aria-busy", String(busy));
}

/* -------------------------------------------------------------------------
 * Workspace
 * ---------------------------------------------------------------------- */

const booleanForm = $("#boolean-form");
const expressionInput = $("#boolean-expression");
const expressionError = $("#expression-error");
const expressionMarker = $("#expression-marker");
const booleanStatus = $("#boolean-status");
const astTree = $("#ast-tree");
const astCount = $("#ast-count");
const truthVariables = $("#truth-variables");
const truthTableHead = $("#truth-table-head");
const truthTableBody = $("#truth-table-body");
const truthRowCount = $("#truth-row-count");
const truthSelected = $("#truth-selected");
const canonicalMinTerms = $("#canonical-minterms");
const canonicalMaxTerms = $("#canonical-maxterms");
const canonicalSop = $("#canonical-sop");
const canonicalPos = $("#canonical-pos");
const canonicalVariableCount = $("#canonical-variable-count");

let workspaceRequest = 0;
let currentVariables = [];
let currentRows = [];
let selectedAssignment = {};
let selectedIndex = null;

function clearExpressionError() {
    expressionError.textContent = "";
    expressionError.hidden = true;
    expressionMarker.textContent = "";
    expressionMarker.hidden = true;
    expressionInput.removeAttribute("aria-invalid");
}

function showExpressionError(error) {
    const suffix = Number.isInteger(error.column)
        ? ` (column ${error.column})`
        : "";
    expressionError.textContent = `${error.message}${suffix}`;
    expressionError.hidden = false;
    expressionInput.setAttribute("aria-invalid", "true");
    setStatus(booleanStatus, "Parse failed", "error");

    if (Number.isInteger(error.position)) {
        const source = expressionInput.value;
        expressionMarker.textContent =
            source + "\n" + " ".repeat(Math.max(0, error.position)) + "^";
        expressionMarker.hidden = false;
    } else {
        expressionMarker.hidden = true;
    }
}

function clearWorkspace() {
    astTree.innerHTML = '<p class="empty-lab">No parsed expression.</p>';
    astCount.textContent = "— nodes";
    truthVariables.replaceChildren();
    truthTableHead.replaceChildren();
    truthTableBody.replaceChildren();
    truthRowCount.textContent = "— rows";
    truthSelected.textContent = "Select a row or toggle a variable.";
    canonicalMinTerms.textContent = "—";
    canonicalMaxTerms.textContent = "—";
    canonicalSop.textContent = "—";
    canonicalPos.textContent = "—";
    canonicalVariableCount.textContent = "— variables";
    currentVariables = [];
    currentRows = [];
    selectedAssignment = {};
    selectedIndex = null;
}

function astKind(node) {
    switch (node.type) {
        case "variable":
            return "VAR";
        case "constant":
            return "CONST";
        case "not":
            return "NOT";
        case "and":
            return "AND";
        case "or":
            return "OR";
        case "xor":
            return "XOR";
        default:
            return String(node.type || "NODE").toUpperCase();
    }
}

function astSymbol(node) {
    switch (node.type) {
        case "variable":
            return node.name;
        case "constant":
            return node.value ? "1" : "0";
        case "not":
            return "!";
        case "and":
            return "&";
        case "or":
            return "|";
        case "xor":
            return "^";
        default:
            return "";
    }
}

function renderAstNode(node) {
    const wrapper = document.createElement("div");
    wrapper.className = `ast-node is-${node.type}`;

    const head = document.createElement("div");
    head.className = "ast-node-head";
    const kind = document.createElement("span");
    kind.className = "ast-kind";
    kind.textContent = astKind(node);
    const symbol = document.createElement("span");
    symbol.className = "ast-symbol";
    if (node.type === "and") {
        symbol.textContent = "&";
    } else if (node.type === "constant") {
        symbol.textContent = node.value ? "1" : "0";
    } else if (node.type === "variable") {
        symbol.textContent = node.name;
    } else {
        symbol.textContent = astSymbol(node);
    }
    head.append(kind, symbol);
    wrapper.append(head);

    if (node.type === "not" && node.operand) {
        const child = document.createElement("div");
        child.className = "ast-single-child";
        child.append(renderAstNode(node.operand));
        wrapper.append(child);
    }

    if (node.left || node.right) {
        const children = document.createElement("div");
        children.className = "ast-children";

        for (const [label, childNode] of [["L", node.left], ["R", node.right]]) {
            if (!childNode) {
                continue;
            }
            const branch = document.createElement("div");
            branch.className = "ast-branch";
            const labelNode = document.createElement("span");
            labelNode.className = "ast-branch-label";
            labelNode.textContent = label;
            branch.append(labelNode, renderAstNode(childNode));
            children.append(branch);
        }

        wrapper.append(children);
    }

    return wrapper;
}

function countAstNodes(node) {
    if (!node || typeof node !== "object") {
        return 0;
    }
    return 1 + countAstNodes(node.operand)
        + countAstNodes(node.left)
        + countAstNodes(node.right);
}

function renderAst(node) {
    astTree.replaceChildren(renderAstNode(node));
    astCount.textContent = `${countAstNodes(node)} nodes`;
}

function updateTruthToggles(focusVariable = null) {
    truthVariables.replaceChildren();

    currentVariables.forEach((variable) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "truth-toggle";
        button.dataset.value = selectedAssignment[variable] ? "1" : "0";
        button.textContent = `${variable} = ${selectedAssignment[variable] ? "1" : "0"}`;
        button.setAttribute(
            "aria-label",
            `Toggle variable ${variable}; current value ${selectedAssignment[variable] ? "1" : "0"}`
        );
        button.setAttribute(
            "aria-pressed",
            selectedAssignment[variable] ? "true" : "false"
        );
        button.addEventListener("click", () => {
            const restoreFocus = document.activeElement === button;
            selectedAssignment[variable] = !selectedAssignment[variable];
            selectMatchingRow(restoreFocus ? variable : null);
        });
        truthVariables.append(button);
        if (variable === focusVariable) {
            button.focus();
        }
    });
}

function rowMatchesSelection(row) {
    return currentVariables.every(
        (variable) => row.assignment[variable] === selectedAssignment[variable]
    );
}

function renderTruthTable() {
    truthTableHead.replaceChildren();
    truthTableBody.replaceChildren();

    const headerRow = document.createElement("tr");
    currentVariables.forEach((variable) => {
        const cell = document.createElement("th");
        cell.scope = "col";
        cell.textContent = variable;
        headerRow.append(cell);
    });
    const resultHeader = document.createElement("th");
    resultHeader.scope = "col";
    resultHeader.textContent = "Result";
    headerRow.append(resultHeader);
    truthTableHead.append(headerRow);

    currentRows.forEach((row) => {
        const tableRow = document.createElement("tr");
        tableRow.tabIndex = 0;
        tableRow.dataset.index = String(row.index);
        if (row.index === selectedIndex) {
            tableRow.classList.add("is-selected");
        }
        tableRow.setAttribute(
            "aria-label",
            `${currentVariables.map(
                (variable) => `${variable}=${row.assignment[variable] ? 1 : 0}`
            ).join(", ")}; result ${row.result ? 1 : 0}`
        );

        currentVariables.forEach((variable) => {
            const cell = document.createElement("td");
            cell.textContent = row.assignment[variable] ? "1" : "0";
            tableRow.append(cell);
        });

        const result = document.createElement("td");
        result.textContent = row.result ? "1" : "0";
        if (row.result) {
            result.classList.add("result-one");
        }
        tableRow.append(result);

        const choose = () => {
            const restoreFocus = document.activeElement === tableRow;
            selectedIndex = row.index;
            selectedAssignment = { ...row.assignment };
            updateTruthToggles();
            renderTruthTable();
            updateTruthSummary();

            if (restoreFocus) {
                const selectedRow = truthTableBody.querySelector(
                    `tr[data-index="${row.index}"]`
                );
                selectedRow?.focus();
            }
        };

        tableRow.addEventListener("click", choose);
        tableRow.addEventListener("keydown", (event) => {
            if (event.key === "Enter" || event.key === " ") {
                event.preventDefault();
                choose();
            }
        });

        truthTableBody.append(tableRow);
    });

    truthRowCount.textContent = `${currentRows.length} rows`;
}

function updateTruthSummary() {
    if (selectedIndex === null || currentRows.length === 0) {
        truthSelected.textContent = "Select a row or toggle a variable.";
        return;
    }

    const row = currentRows.find((candidate) => candidate.index === selectedIndex);
    if (!row) {
        truthSelected.textContent = "Select a row or toggle a variable.";
        return;
    }

    const assignment = currentVariables.length === 0
        ? "No variables"
        : currentVariables
            .map((variable) => `${variable}=${row.assignment[variable] ? "1" : "0"}`)
            .join(" · ");
    truthSelected.textContent =
        `${assignment} → ${row.result ? "1" : "0"} (row ${row.index})`;
}

function selectMatchingRow(focusVariable = null) {
    const row = currentRows.find(rowMatchesSelection);
    selectedIndex = row ? row.index : null;
    updateTruthToggles(focusVariable);
    renderTruthTable();
    updateTruthSummary();
}

function initializeSelection() {
    selectedAssignment = {};
    currentVariables.forEach((variable) => {
        selectedAssignment[variable] = false;
    });
    selectedIndex = 0;
    updateTruthToggles();
    renderTruthTable();
    updateTruthSummary();
}

function renderCanonical(canonical) {
    canonicalMinTerms.textContent = canonical.minterms.length === 0
        ? "∅"
        : canonical.minterms.join(", ");
    canonicalMaxTerms.textContent = canonical.maxterms.length === 0
        ? "∅"
        : canonical.maxterms.join(", ");
    canonicalSop.textContent = canonical.sop;
    canonicalPos.textContent = canonical.pos;
    canonicalVariableCount.textContent =
        `${canonical.variables.length} variable${canonical.variables.length === 1 ? "" : "s"}`;
}

async function refreshWorkspace() {
    const request = ++workspaceRequest;
    clearExpressionError();

    const expression = expressionInput.value;
    if (expression.trim() === "") {
        setBusy(booleanForm, false);
        clearWorkspace();
        setStatus(booleanStatus, "Waiting", "warning");
        showExpressionError(new BooleanApiError(
            "Enter a Boolean expression.", 0, "invalid_request"));
        return;
    }

    if (expression.length > 1024) {
        setBusy(booleanForm, false);
        clearWorkspace();
        setStatus(booleanStatus, "Too long", "error");
        showExpressionError(new BooleanApiError(
            "Expression exceeds the 1024-character limit.", 0, "limit"));
        return;
    }

    setBusy(booleanForm, true);
    setStatus(booleanStatus, "Parsing…");

    try {
        const [table, canonical] = await Promise.all([
            fetchBoolean("/api/v1/boolean/truth-table", { expression }),
            fetchBoolean("/api/v1/boolean/canonical", { expression })
        ]);

        if (request !== workspaceRequest) {
            return;
        }

        currentVariables = [...table.variables];
        currentRows = [...table.rows];
        renderAst(table.ast);
        initializeSelection();
        renderCanonical(canonical);
        setStatus(booleanStatus, "Parsed", "ok");
    } catch (error) {
        if (request !== workspaceRequest) {
            return;
        }
        clearWorkspace();
        showExpressionError(error);
    } finally {
        if (request === workspaceRequest) {
            setBusy(booleanForm, false);
        }
    }
}

const scheduleWorkspace = debounce(() => refreshWorkspace());

expressionInput.addEventListener("input", scheduleWorkspace);
booleanForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshWorkspace();
});

document.querySelectorAll(".expression-example").forEach((button) => {
    button.addEventListener("click", () => {
        expressionInput.value = button.dataset.expression;
        refreshWorkspace();
    });
});

/* -------------------------------------------------------------------------
 * Equivalence
 * ---------------------------------------------------------------------- */

const equivalenceForm = $("#equivalence-form");
const equivalenceLeft = $("#equivalence-left");
const equivalenceRight = $("#equivalence-right");
const equivalenceStatus = $("#equivalence-status");
const equivalenceError = $("#equivalence-error");
const equivalenceVerdict = $("#equivalence-verdict");
const equivalenceNormalized = $("#equivalence-normalized");
const equivalenceLeftNormalized = $("#equivalence-left-normalized");
const equivalenceRightNormalized = $("#equivalence-right-normalized");
const counterexample = $("#counterexample");
const counterexampleBody = $("#counterexample-body");

let equivalenceRequest = 0;

function clearEquivalenceError() {
    equivalenceError.textContent = "";
    equivalenceError.hidden = true;
    equivalenceLeft.removeAttribute("aria-invalid");
    equivalenceRight.removeAttribute("aria-invalid");
}

function showEquivalenceError(error) {
    equivalenceError.textContent = Number.isInteger(error.column)
        ? `${error.message} (column ${error.column})`
        : error.message;
    equivalenceError.hidden = false;
    if (error.message.startsWith("Left expression")) {
        equivalenceLeft.setAttribute("aria-invalid", "true");
    } else if (error.message.startsWith("Right expression")) {
        equivalenceRight.setAttribute("aria-invalid", "true");
    }
    setStatus(equivalenceStatus, "Invalid", "error");
    equivalenceVerdict.textContent = "Could not compare expressions.";
    equivalenceVerdict.className = "equivalence-verdict";
    equivalenceNormalized.hidden = true;
    counterexample.hidden = true;
}

function renderEquivalence(result) {
    equivalenceNormalized.hidden = false;
    equivalenceLeftNormalized.textContent = result.leftNormalized;
    equivalenceRightNormalized.textContent = result.rightNormalized;

    if (result.equivalent) {
        setStatus(equivalenceStatus, "Equivalent", "ok");
        equivalenceVerdict.textContent =
            "Equivalent over every assignment.";
        equivalenceVerdict.className = "equivalence-verdict is-equivalent";
        counterexample.hidden = true;
        return;
    }

    setStatus(equivalenceStatus, "Not equivalent", "warning");
    equivalenceVerdict.textContent =
        "A counterexample distinguishes the expressions.";
    equivalenceVerdict.className = "equivalence-verdict is-different";
    counterexample.hidden = false;
    counterexampleBody.replaceChildren();

    const chips = document.createElement("div");
    chips.className = "counterexample-row";
    Object.entries(result.counterexample.assignment).forEach(
        ([variable, value]) => {
            const chip = document.createElement("span");
            chip.className = "counterexample-chip";
            chip.textContent = `${variable} = ${value ? "1" : "0"}`;
            chips.append(chip);
        }
    );

    const values = document.createElement("div");
    values.className = "counterexample-results";
    const leftValue = document.createElement("span");
    leftValue.append("Left → ");
    const leftCode = document.createElement("code");
    leftCode.textContent = result.counterexample.left ? "1" : "0";
    leftValue.append(leftCode);
    const rightValue = document.createElement("span");
    rightValue.append("Right → ");
    const rightCode = document.createElement("code");
    rightCode.textContent = result.counterexample.right ? "1" : "0";
    rightValue.append(rightCode);
    values.append(leftValue, rightValue);

    counterexampleBody.append(chips, values);
}

async function refreshEquivalence() {
    const request = ++equivalenceRequest;
    clearEquivalenceError();

    const left = equivalenceLeft.value;
    const right = equivalenceRight.value;

    if (left.trim() === "" || right.trim() === "") {
        setBusy(equivalenceForm, false);
        setStatus(equivalenceStatus, "Waiting", "warning");
        equivalenceVerdict.textContent =
            "Provide both expressions to compare them.";
        equivalenceVerdict.className = "equivalence-verdict";
        equivalenceNormalized.hidden = true;
        counterexample.hidden = true;
        return;
    }

    setBusy(equivalenceForm, true);
    setStatus(equivalenceStatus, "Comparing…");

    try {
        const result = await fetchBoolean(
            "/api/v1/boolean/equivalent",
            { leftExpression: left, rightExpression: right }
        );

        if (request !== equivalenceRequest) {
            return;
        }
        renderEquivalence(result);
    } catch (error) {
        if (request !== equivalenceRequest) {
            return;
        }
        showEquivalenceError(error);
    } finally {
        if (request === equivalenceRequest) {
            setBusy(equivalenceForm, false);
        }
    }
}

const scheduleEquivalence = debounce(() => refreshEquivalence());
equivalenceLeft.addEventListener("input", scheduleEquivalence);
equivalenceRight.addEventListener("input", scheduleEquivalence);
equivalenceForm.addEventListener("submit", (event) => {
    event.preventDefault();
    refreshEquivalence();
});

refreshWorkspace();
refreshEquivalence();
