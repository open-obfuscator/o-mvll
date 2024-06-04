"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.copyWeb = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const errors_1 = require("../errors");
const node_1 = require("../util/node");
async function copyWeb(config) {
    if (config.app.bundledWebRuntime) {
        const runtimePath = (0, node_1.resolveNode)(config.app.rootDir, '@capacitor/core', 'dist', 'capacitor.js');
        if (!runtimePath) {
            (0, errors_1.fatal)(`Unable to find ${colors_1.default.strong('node_modules/@capacitor/core/dist/capacitor.js')}.\n` + `Are you sure ${colors_1.default.strong('@capacitor/core')} is installed?`);
        }
        return (0, common_1.runTask)(`Copying ${colors_1.default.strong('capacitor.js')} to web dir`, () => {
            return (0, utils_fs_1.copy)(runtimePath, (0, path_1.join)(config.app.webDirAbs, 'capacitor.js'));
        });
    }
}
exports.copyWeb = copyWeb;
