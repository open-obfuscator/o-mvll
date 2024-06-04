"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.serveCommand = void 0;
const tslib_1 = require("tslib");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const errors_1 = require("../errors");
async function serveCommand() {
    (0, errors_1.fatal)(`The serve command has been removed.\n` +
        `Use a third-party tool for serving single page apps, such as ${colors_1.default.strong('serve')}: ${colors_1.default.strong('https://www.npmjs.com/package/serve')}`);
}
exports.serveCommand = serveCommand;
