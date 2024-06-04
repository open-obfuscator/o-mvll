"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.formatJSObject = void 0;
const tslib_1 = require("tslib");
const util_1 = tslib_1.__importDefault(require("util"));
function formatJSObject(o) {
    try {
        o = JSON.parse(JSON.stringify(o));
    }
    catch (e) {
        throw new Error(`Cannot parse object as JSON: ${e.stack ? e.stack : e}`);
    }
    return util_1.default.inspect(o, {
        compact: false,
        breakLength: Infinity,
        depth: Infinity,
        maxArrayLength: Infinity,
        maxStringLength: Infinity,
    });
}
exports.formatJSObject = formatJSObject;
