"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isInteractive = exports.checkInteractive = void 0;
const tslib_1 = require("tslib");
const utils_terminal_1 = require("@ionic/utils-terminal");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const log_1 = require("../log");
// Given input variables to a command, make sure all are provided if the terminal
// is not interactive (because we won't be able to prompt the user)
const checkInteractive = (...args) => {
    if ((0, exports.isInteractive)()) {
        return true;
    }
    // Fail if no args are provided, treat this as just a check of whether the term is
    // interactive or not.
    if (!args.length) {
        return false;
    }
    // Make sure none of the provided args are empty, otherwise print the interactive
    // warning and return false
    if (args.filter(arg => !arg).length) {
        log_1.logger.error(`Non-interactive shell detected.\n` +
            `Run the command with ${colors_1.default.input('--help')} to see a list of arguments that must be provided.`);
        return false;
    }
    return true;
};
exports.checkInteractive = checkInteractive;
const isInteractive = () => utils_terminal_1.TERMINAL_INFO.tty && !utils_terminal_1.TERMINAL_INFO.ci;
exports.isInteractive = isInteractive;
