"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getAllElements = exports.getFilePath = exports.getAssets = exports.getJSModules = exports.getPluginType = exports.getPlatformElement = exports.getPluginPlatform = exports.printPlugins = exports.fixName = exports.getDependencies = exports.resolvePlugin = exports.getPlugins = exports.getIncludedPluginPackages = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const errors_1 = require("./errors");
const log_1 = require("./log");
const node_1 = require("./util/node");
const xml_1 = require("./util/xml");
function getIncludedPluginPackages(config, platform) {
    var _a, _b, _c, _d;
    const { extConfig } = config.app;
    switch (platform) {
        case 'android':
            return (_b = (_a = extConfig.android) === null || _a === void 0 ? void 0 : _a.includePlugins) !== null && _b !== void 0 ? _b : extConfig.includePlugins;
        case 'ios':
            return (_d = (_c = extConfig.ios) === null || _c === void 0 ? void 0 : _c.includePlugins) !== null && _d !== void 0 ? _d : extConfig.includePlugins;
    }
}
exports.getIncludedPluginPackages = getIncludedPluginPackages;
async function getPlugins(config, platform) {
    var _a;
    const possiblePlugins = (_a = getIncludedPluginPackages(config, platform)) !== null && _a !== void 0 ? _a : getDependencies(config);
    const resolvedPlugins = await Promise.all(possiblePlugins.map(async (p) => resolvePlugin(config, p)));
    return resolvedPlugins.filter((p) => !!p);
}
exports.getPlugins = getPlugins;
async function resolvePlugin(config, name) {
    try {
        const packagePath = (0, node_1.resolveNode)(config.app.rootDir, name, 'package.json');
        if (!packagePath) {
            (0, errors_1.fatal)(`Unable to find ${colors_1.default.strong(`node_modules/${name}`)}.\n` +
                `Are you sure ${colors_1.default.strong(name)} is installed?`);
        }
        const rootPath = (0, path_1.dirname)(packagePath);
        const meta = await (0, utils_fs_1.readJSON)(packagePath);
        if (!meta) {
            return null;
        }
        if (meta.capacitor) {
            return {
                id: name,
                name: fixName(name),
                version: meta.version,
                rootPath,
                repository: meta.repository,
                manifest: meta.capacitor,
            };
        }
        const pluginXMLPath = (0, path_1.join)(rootPath, 'plugin.xml');
        const xmlMeta = await (0, xml_1.readXML)(pluginXMLPath);
        return {
            id: name,
            name: fixName(name),
            version: meta.version,
            rootPath: rootPath,
            repository: meta.repository,
            xml: xmlMeta.plugin,
        };
    }
    catch (e) {
        // ignore
    }
    return null;
}
exports.resolvePlugin = resolvePlugin;
function getDependencies(config) {
    var _a, _b;
    return [
        ...Object.keys((_a = config.app.package.dependencies) !== null && _a !== void 0 ? _a : {}),
        ...Object.keys((_b = config.app.package.devDependencies) !== null && _b !== void 0 ? _b : {}),
    ];
}
exports.getDependencies = getDependencies;
function fixName(name) {
    name = name
        .replace(/\//g, '_')
        .replace(/-/g, '_')
        .replace(/@/g, '')
        .replace(/_\w/g, m => m[1].toUpperCase());
    return name.charAt(0).toUpperCase() + name.slice(1);
}
exports.fixName = fixName;
function printPlugins(plugins, platform, type = 'capacitor') {
    if (plugins.length === 0) {
        return;
    }
    let msg;
    const plural = plugins.length === 1 ? '' : 's';
    switch (type) {
        case 'cordova':
            msg = `Found ${plugins.length} Cordova plugin${plural} for ${colors_1.default.strong(platform)}:\n`;
            break;
        case 'incompatible':
            msg = `Found ${plugins.length} incompatible Cordova plugin${plural} for ${colors_1.default.strong(platform)}, skipped install:\n`;
            break;
        case 'capacitor':
            msg = `Found ${plugins.length} Capacitor plugin${plural} for ${colors_1.default.strong(platform)}:\n`;
            break;
    }
    msg += plugins.map(p => `${p.id}${colors_1.default.weak(`@${p.version}`)}`).join('\n');
    log_1.logger.info(msg);
}
exports.printPlugins = printPlugins;
function getPluginPlatform(p, platform) {
    const platforms = p.xml.platform;
    if (platforms) {
        const platforms = p.xml.platform.filter(function (item) {
            return item.$.name === platform;
        });
        return platforms[0];
    }
    return [];
}
exports.getPluginPlatform = getPluginPlatform;
function getPlatformElement(p, platform, elementName) {
    const platformTag = getPluginPlatform(p, platform);
    if (platformTag) {
        const element = platformTag[elementName];
        if (element) {
            return element;
        }
    }
    return [];
}
exports.getPlatformElement = getPlatformElement;
function getPluginType(p, platform) {
    var _a, _b, _c, _d;
    switch (platform) {
        case 'ios':
            return (_b = (_a = p.ios) === null || _a === void 0 ? void 0 : _a.type) !== null && _b !== void 0 ? _b : 0 /* PluginType.Core */;
        case 'android':
            return (_d = (_c = p.android) === null || _c === void 0 ? void 0 : _c.type) !== null && _d !== void 0 ? _d : 0 /* PluginType.Core */;
    }
    return 0 /* PluginType.Core */;
}
exports.getPluginType = getPluginType;
/**
 * Get each JavaScript Module for the given plugin
 */
function getJSModules(p, platform) {
    return getAllElements(p, platform, 'js-module');
}
exports.getJSModules = getJSModules;
/**
 * Get each asset tag for the given plugin
 */
function getAssets(p, platform) {
    return getAllElements(p, platform, 'asset');
}
exports.getAssets = getAssets;
function getFilePath(config, plugin, path) {
    if (path.startsWith('node_modules')) {
        let pathSegments = path.split('/').slice(1);
        if (pathSegments[0].startsWith('@')) {
            pathSegments = [
                pathSegments[0] + '/' + pathSegments[1],
                ...pathSegments.slice(2),
            ];
        }
        const filePath = (0, node_1.resolveNode)(config.app.rootDir, ...pathSegments);
        if (!filePath) {
            throw new Error(`Can't resolve module ${pathSegments[0]}`);
        }
        return filePath;
    }
    return (0, path_1.join)(plugin.rootPath, path);
}
exports.getFilePath = getFilePath;
/**
 * For a given plugin, return all the plugin.xml elements with elementName, checking root and specified platform
 */
function getAllElements(p, platform, elementName) {
    let modules = [];
    if (p.xml[elementName]) {
        modules = modules.concat(p.xml[elementName]);
    }
    const platformModules = getPluginPlatform(p, platform);
    if (platformModules === null || platformModules === void 0 ? void 0 : platformModules[elementName]) {
        modules = modules.concat(platformModules[elementName]);
    }
    return modules;
}
exports.getAllElements = getAllElements;
