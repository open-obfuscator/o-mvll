"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.addIOS = void 0;
const tslib_1 = require("tslib");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const template_1 = require("../util/template");
async function addIOS(config) {
    await (0, common_1.runTask)(`Adding native Xcode project in ${colors_1.default.strong(config.ios.platformDir)}`, () => {
        return (0, template_1.extractTemplate)(config.cli.assets.ios.platformTemplateArchiveAbs, config.ios.platformDirAbs);
    });
}
exports.addIOS = addIOS;
