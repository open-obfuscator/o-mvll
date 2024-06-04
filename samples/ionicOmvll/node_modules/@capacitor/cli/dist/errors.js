"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isFatal = exports.fatal = exports.FatalException = exports.BaseException = void 0;
class BaseException extends Error {
    constructor(message, code) {
        super(message);
        this.message = message;
        this.code = code;
    }
}
exports.BaseException = BaseException;
class FatalException extends BaseException {
    constructor(message, exitCode = 1) {
        super(message, 'FATAL');
        this.message = message;
        this.exitCode = exitCode;
    }
}
exports.FatalException = FatalException;
function fatal(message) {
    throw new FatalException(message);
}
exports.fatal = fatal;
function isFatal(e) {
    return e && e instanceof FatalException;
}
exports.isFatal = isFatal;
