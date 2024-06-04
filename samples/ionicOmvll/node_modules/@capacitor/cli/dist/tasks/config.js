"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.configCommand = void 0;
const tslib_1 = require("tslib");
const util_1 = tslib_1.__importDefault(require("util"));
const log_1 = require("../log");
async function configCommand(config, json) {
    const evaluatedConfig = await deepAwait(config);
    if (json) {
        process.stdout.write(`${JSON.stringify(evaluatedConfig)}\n`);
    }
    else {
        log_1.output.write(`${util_1.default.inspect(evaluatedConfig, { depth: Infinity, colors: true })}\n`);
    }
}
exports.configCommand = configCommand;
async function deepAwait(obj) {
    if (obj &&
        !Array.isArray(obj) &&
        typeof obj === 'object' &&
        obj.constructor === Object) {
        const o = {};
        for (const [k, v] of Object.entries(obj)) {
            o[k] = await deepAwait(v);
        }
        return o;
    }
    else {
        return await obj;
    }
}
