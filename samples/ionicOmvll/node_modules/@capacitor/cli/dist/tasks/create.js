"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.createCommand = void 0;
const tslib_1 = require("tslib");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const errors_1 = require("../errors");
async function createCommand() {
    (0, errors_1.fatal)(`The create command has been removed.\n` +
        `Use ${colors_1.default.input('npm init @capacitor/app')}`);
}
exports.createCommand = createCommand;
