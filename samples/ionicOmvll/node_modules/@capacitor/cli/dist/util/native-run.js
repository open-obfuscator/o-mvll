"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getPlatformTargets = exports.runNativeRun = void 0;
const tslib_1 = require("tslib");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const errors_1 = require("../errors");
const node_1 = require("./node");
const subprocess_1 = require("./subprocess");
async function runNativeRun(args, options = {}) {
    const p = (0, node_1.resolveNode)(__dirname, (0, path_1.dirname)('native-run/package'), 'bin/native-run');
    if (!p) {
        (0, errors_1.fatal)(`${colors_1.default.input('native-run')} not found.`);
    }
    return await (0, subprocess_1.runCommand)(p, args, options);
}
exports.runNativeRun = runNativeRun;
async function getPlatformTargets(platformName) {
    const errors = [];
    try {
        const output = await runNativeRun([platformName, '--list', '--json']);
        const parsedOutput = JSON.parse(output);
        if (parsedOutput.devices.length || parsedOutput.virtualDevices.length) {
            return [
                ...parsedOutput.devices.map((t) => ({ ...t, virtual: false })),
                ...parsedOutput.virtualDevices.map((t) => ({
                    ...t,
                    virtual: true,
                })),
            ];
        }
        else {
            parsedOutput.errors.map((e) => {
                errors.push(e);
            });
        }
    }
    catch (e) {
        const err = JSON.parse(e);
        errors.push(err);
    }
    const plural = errors.length > 1 ? 's' : '';
    const errMsg = `${colors_1.default.strong('native-run')} failed with error${plural}\n
  ${errors
        .map((e) => {
        return `\t${colors_1.default.strong(e.code)}: ${e.error}`;
    })
        .join('\n')}
  \n\tMore details for this error${plural} may be available online: ${colors_1.default.strong('https://github.com/ionic-team/native-run/wiki/Android-Errors')}
  `;
    throw errMsg;
}
exports.getPlatformTargets = getPlatformTargets;
