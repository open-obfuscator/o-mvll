"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.list = exports.listCommand = void 0;
const tslib_1 = require("tslib");
const common_1 = require("../android/common");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_2 = require("../common");
const errors_1 = require("../errors");
const common_3 = require("../ios/common");
const log_1 = require("../log");
const plugin_1 = require("../plugin");
const promise_1 = require("../util/promise");
async function listCommand(config, selectedPlatformName) {
    var _a;
    const platforms = await (0, common_2.selectPlatforms)(config, selectedPlatformName);
    try {
        await (0, promise_1.allSerial)(platforms.map(platformName => () => list(config, platformName)));
    }
    catch (e) {
        if ((0, errors_1.isFatal)(e)) {
            throw e;
        }
        log_1.logger.error((_a = e.stack) !== null && _a !== void 0 ? _a : e);
    }
}
exports.listCommand = listCommand;
async function list(config, platform) {
    const allPlugins = await (0, plugin_1.getPlugins)(config, platform);
    let plugins = [];
    if (platform === config.ios.name) {
        plugins = await (0, common_3.getIOSPlugins)(allPlugins);
    }
    else if (platform === config.android.name) {
        plugins = await (0, common_1.getAndroidPlugins)(allPlugins);
    }
    else if (platform === config.web.name) {
        log_1.logger.info(`Listing plugins for ${colors_1.default.input(platform)} is not possible.`);
        return;
    }
    else {
        throw `Platform ${colors_1.default.input(platform)} is not valid.`;
    }
    const capacitorPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 0 /* PluginType.Core */);
    (0, plugin_1.printPlugins)(capacitorPlugins, platform);
    const cordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 1 /* PluginType.Cordova */);
    (0, plugin_1.printPlugins)(cordovaPlugins, platform, 'cordova');
    const incompatibleCordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 2 /* PluginType.Incompatible */);
    (0, plugin_1.printPlugins)(incompatibleCordovaPlugins, platform, 'incompatible');
}
exports.list = list;
