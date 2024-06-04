"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.writeConfig = exports.readConfig = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const debug_1 = tslib_1.__importDefault(require("debug"));
const path_1 = require("path");
const cli_1 = require("./util/cli");
const uuid_1 = require("./util/uuid");
const debug = (0, debug_1.default)('capacitor:sysconfig');
const SYSCONFIG_FILE = 'sysconfig.json';
const SYSCONFIG_PATH = (0, path_1.resolve)(cli_1.ENV_PATHS.config, SYSCONFIG_FILE);
async function readConfig() {
    debug('Reading from %O', SYSCONFIG_PATH);
    try {
        return await (0, utils_fs_1.readJSON)(SYSCONFIG_PATH);
    }
    catch (e) {
        if (e.code !== 'ENOENT') {
            throw e;
        }
        const sysconfig = {
            machine: (0, uuid_1.uuidv4)(),
        };
        await writeConfig(sysconfig);
        return sysconfig;
    }
}
exports.readConfig = readConfig;
async function writeConfig(sysconfig) {
    debug('Writing to %O', SYSCONFIG_PATH);
    await (0, utils_fs_1.mkdirp)((0, path_1.dirname)(SYSCONFIG_PATH));
    await (0, utils_fs_1.writeJSON)(SYSCONFIG_PATH, sysconfig, { spaces: '\t' });
}
exports.writeConfig = writeConfig;
