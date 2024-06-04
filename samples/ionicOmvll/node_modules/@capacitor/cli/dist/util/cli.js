"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.wrapAction = exports.ENV_PATHS = void 0;
const tslib_1 = require("tslib");
const env_paths_1 = tslib_1.__importDefault(require("env-paths"));
const errors_1 = require("../errors");
const log_1 = require("../log");
exports.ENV_PATHS = (0, env_paths_1.default)('capacitor', { suffix: '' });
function wrapAction(action) {
    return async (...args) => {
        try {
            await action(...args);
        }
        catch (e) {
            if ((0, errors_1.isFatal)(e)) {
                process.exitCode = e.exitCode;
                log_1.logger.error(e.message);
            }
            else {
                throw e;
            }
        }
    };
}
exports.wrapAction = wrapAction;
