"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.newPluginCommand = void 0;
const tslib_1 = require("tslib");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const errors_1 = require("../errors");
async function newPluginCommand() {
    (0, errors_1.fatal)(`The plugin:generate command has been removed.\n` +
        `Use ${colors_1.default.input('npm init @capacitor/plugin')}`);
}
exports.newPluginCommand = newPluginCommand;
