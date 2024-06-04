"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.openIOS = void 0;
const tslib_1 = require("tslib");
const open_1 = tslib_1.__importDefault(require("open"));
const common_1 = require("../common");
const spm_1 = require("../util/spm");
async function openIOS(config) {
    if ((await (0, spm_1.checkPackageManager)(config)) == 'SPM') {
        await (0, open_1.default)(config.ios.nativeXcodeProjDirAbs, { wait: false });
    }
    else {
        await (0, open_1.default)(await config.ios.nativeXcodeWorkspaceDirAbs, { wait: false });
    }
    await (0, common_1.wait)(3000);
}
exports.openIOS = openIOS;
