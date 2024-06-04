"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.editProjectSettingsIOS = exports.resolvePlugin = exports.getIOSPlugins = exports.checkCocoaPods = exports.checkBundler = exports.checkIOSPackage = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const child_process_1 = require("child_process");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const cordova_1 = require("../cordova");
const log_1 = require("../log");
const plugin_1 = require("../plugin");
const subprocess_1 = require("../util/subprocess");
async function checkIOSPackage(config) {
    return (0, common_1.checkCapacitorPlatform)(config, 'ios');
}
exports.checkIOSPackage = checkIOSPackage;
function execBundler() {
    try {
        const bundleOutput = (0, child_process_1.execSync)('bundle &> /dev/null ; echo $?');
        return parseInt(bundleOutput.toString());
    }
    catch (e) {
        return -1;
    }
}
async function checkBundler(config) {
    if (config.cli.os === "mac" /* OS.Mac */) {
        let bundlerResult = execBundler();
        if (bundlerResult === 1) {
            // Bundler version is outdated
            log_1.logger.info(`Using ${colors_1.default.strong('Gemfile')}: Bundler update needed...`);
            await (0, subprocess_1.runCommand)('gem', ['install', 'bundler']);
            bundlerResult = execBundler();
        }
        if (bundlerResult === 0) {
            // Bundler in use, all gems current
            log_1.logger.info(`Using ${colors_1.default.strong('Gemfile')}: RubyGems bundle installed`);
        }
    }
    return null;
}
exports.checkBundler = checkBundler;
async function checkCocoaPods(config) {
    if (!(await (0, subprocess_1.isInstalled)(await config.ios.podPath)) &&
        config.cli.os === "mac" /* OS.Mac */) {
        return (`CocoaPods is not installed.\n` +
            `See this install guide: ${colors_1.default.strong('https://capacitorjs.com/docs/getting-started/environment-setup#homebrew')}`);
    }
    return null;
}
exports.checkCocoaPods = checkCocoaPods;
async function getIOSPlugins(allPlugins) {
    const resolved = await Promise.all(allPlugins.map(async (plugin) => await resolvePlugin(plugin)));
    return resolved.filter((plugin) => !!plugin);
}
exports.getIOSPlugins = getIOSPlugins;
async function resolvePlugin(plugin) {
    var _a, _b;
    const platform = 'ios';
    if ((_a = plugin.manifest) === null || _a === void 0 ? void 0 : _a.ios) {
        plugin.ios = {
            name: plugin.name,
            type: 0 /* PluginType.Core */,
            path: (_b = plugin.manifest.ios.src) !== null && _b !== void 0 ? _b : platform,
        };
    }
    else if (plugin.xml) {
        plugin.ios = {
            name: plugin.name,
            type: 1 /* PluginType.Cordova */,
            path: 'src/' + platform,
        };
        if ((0, cordova_1.getIncompatibleCordovaPlugins)(platform).includes(plugin.id) ||
            !(0, plugin_1.getPluginPlatform)(plugin, platform)) {
            plugin.ios.type = 2 /* PluginType.Incompatible */;
        }
    }
    else {
        return null;
    }
    return plugin;
}
exports.resolvePlugin = resolvePlugin;
/**
 * Update the native project files with the desired app id and app name
 */
async function editProjectSettingsIOS(config) {
    const appId = config.app.appId;
    const appName = config.app.appName
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
    const pbxPath = `${config.ios.nativeXcodeProjDirAbs}/project.pbxproj`;
    const plistPath = (0, path_1.resolve)(config.ios.nativeTargetDirAbs, 'Info.plist');
    let plistContent = await (0, utils_fs_1.readFile)(plistPath, { encoding: 'utf-8' });
    plistContent = plistContent.replace(/<key>CFBundleDisplayName<\/key>[\s\S]?\s+<string>([^<]*)<\/string>/, `<key>CFBundleDisplayName</key>\n        <string>${appName}</string>`);
    let pbxContent = await (0, utils_fs_1.readFile)(pbxPath, { encoding: 'utf-8' });
    pbxContent = pbxContent.replace(/PRODUCT_BUNDLE_IDENTIFIER = ([^;]+)/g, `PRODUCT_BUNDLE_IDENTIFIER = ${appId}`);
    await (0, utils_fs_1.writeFile)(plistPath, plistContent, { encoding: 'utf-8' });
    await (0, utils_fs_1.writeFile)(pbxPath, pbxContent, { encoding: 'utf-8' });
}
exports.editProjectSettingsIOS = editProjectSettingsIOS;
