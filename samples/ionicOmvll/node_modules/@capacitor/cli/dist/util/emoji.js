"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.emoji = void 0;
// Emoji falback, right now just uses fallback on windows,
// but could expand to be more sophisticated to allow emoji
// on Hyper term on windows, for example.
const emoji = (x, fallback) => {
    if (process.platform === 'win32') {
        return fallback;
    }
    return x;
};
exports.emoji = emoji;
