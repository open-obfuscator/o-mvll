"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.tryFn = void 0;
const tryFn = async (fn, ...args) => {
    try {
        return await fn(...args);
    }
    catch {
        // ignore
    }
    return null;
};
exports.tryFn = tryFn;
