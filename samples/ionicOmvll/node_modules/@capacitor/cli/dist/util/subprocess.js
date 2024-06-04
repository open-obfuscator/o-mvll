"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isInstalled = exports.getCommandOutput = exports.runCommand = void 0;
const utils_subprocess_1 = require("@ionic/utils-subprocess");
async function runCommand(command, args, options = {}) {
    const p = new utils_subprocess_1.Subprocess(command, args, options);
    try {
        return await p.output();
    }
    catch (e) {
        if (e instanceof utils_subprocess_1.SubprocessError) {
            // old behavior of just throwing the stdout/stderr strings
            throw e.output
                ? e.output
                : e.code
                    ? e.code
                    : e.error
                        ? e.error.message
                        : 'Unknown error';
        }
        throw e;
    }
}
exports.runCommand = runCommand;
async function getCommandOutput(command, args, options = {}) {
    try {
        return (await runCommand(command, args, options)).trim();
    }
    catch (e) {
        return null;
    }
}
exports.getCommandOutput = getCommandOutput;
async function isInstalled(command) {
    try {
        await (0, utils_subprocess_1.which)(command);
    }
    catch (e) {
        return false;
    }
    return true;
}
exports.isInstalled = isInstalled;
