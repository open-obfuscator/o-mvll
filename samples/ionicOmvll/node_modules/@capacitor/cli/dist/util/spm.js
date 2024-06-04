"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.generatePackageFile = exports.findPackageSwiftFile = exports.checkPackageManager = void 0;
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const log_1 = require("../log");
async function checkPackageManager(config) {
    const iosDirectory = config.ios.nativeProjectDirAbs;
    if ((0, utils_fs_1.existsSync)((0, path_1.resolve)(iosDirectory, 'CapApp-SPM'))) {
        return 'SPM';
    }
    return 'Cocoapods';
}
exports.checkPackageManager = checkPackageManager;
async function findPackageSwiftFile(config) {
    const packageDirectory = (0, path_1.resolve)(config.ios.nativeProjectDirAbs, 'CapApp-SPM');
    return (0, path_1.resolve)(packageDirectory, 'Package.swift');
}
exports.findPackageSwiftFile = findPackageSwiftFile;
async function generatePackageFile(config, plugins) {
    const packageSwiftFile = await findPackageSwiftFile(config);
    try {
        log_1.logger.warn('SPM Support is still experimental');
        const textToWrite = generatePackageText(config, plugins);
        (0, utils_fs_1.writeFileSync)(packageSwiftFile, textToWrite);
    }
    catch (err) {
        log_1.logger.error(`Unable to write to ${packageSwiftFile}. Verify it is not already open. \n Error: ${err}`);
    }
}
exports.generatePackageFile = generatePackageFile;
function generatePackageText(config, plugins) {
    var _a, _b, _c;
    let packageSwiftText = `// swift-tools-version: 5.9
import PackageDescription

// DO NOT MODIFY THIS FILE - managed by Capacitor CLI commands
let package = Package(
    name: "CapApp-SPM",
    platforms: [.iOS(.v13)],
    products: [
        .library(
            name: "CapApp-SPM",
            targets: ["CapApp-SPM"])
    ],
    dependencies: [
        .package(url: "https://github.com/ionic-team/capacitor-swift-pm.git", branch: "main")`;
    for (const plugin of plugins) {
        const relPath = (0, path_1.relative)(config.ios.nativeXcodeProjDirAbs, plugin.rootPath);
        packageSwiftText += `,\n        .package(name: "${(_a = plugin.ios) === null || _a === void 0 ? void 0 : _a.name}", path: "${relPath}")`;
    }
    packageSwiftText += `
    ],
    targets: [
        .target(
            name: "CapApp-SPM",
            dependencies: [
                .product(name: "Capacitor", package: "capacitor-swift-pm"),
                .product(name: "Cordova", package: "capacitor-swift-pm")`;
    for (const plugin of plugins) {
        packageSwiftText += `,\n                .product(name: "${(_b = plugin.ios) === null || _b === void 0 ? void 0 : _b.name}", package: "${(_c = plugin.ios) === null || _c === void 0 ? void 0 : _c.name}")`;
    }
    packageSwiftText += `
            ]
        )
    ]
)
`;
    return packageSwiftText;
}
