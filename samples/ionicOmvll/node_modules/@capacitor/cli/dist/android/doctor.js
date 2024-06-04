"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.doctorAndroid = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const errors_1 = require("../errors");
const log_1 = require("../log");
const xml_1 = require("../util/xml");
async function doctorAndroid(config) {
    var _a;
    try {
        await (0, common_1.check)([
            checkAndroidInstalled,
            () => checkGradlew(config),
            () => checkAppSrcDirs(config),
        ]);
        (0, log_1.logSuccess)('Android looking great! 👌');
    }
    catch (e) {
        if (!(0, errors_1.isFatal)(e)) {
            (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
        }
        throw e;
    }
}
exports.doctorAndroid = doctorAndroid;
async function checkAppSrcDirs(config) {
    if (!(await (0, utils_fs_1.pathExists)(config.android.appDirAbs))) {
        return `${colors_1.default.strong(config.android.appDir)} directory is missing in ${colors_1.default.strong(config.android.platformDir)}`;
    }
    if (!(await (0, utils_fs_1.pathExists)(config.android.srcMainDirAbs))) {
        return `${colors_1.default.strong(config.android.srcMainDir)} directory is missing in ${colors_1.default.strong(config.android.platformDir)}`;
    }
    if (!(await (0, utils_fs_1.pathExists)(config.android.assetsDirAbs))) {
        return `${colors_1.default.strong(config.android.assetsDir)} directory is missing in ${colors_1.default.strong(config.android.platformDir)}`;
    }
    if (!(await (0, utils_fs_1.pathExists)(config.android.webDirAbs))) {
        return `${colors_1.default.strong(config.android.webDir)} directory is missing in ${colors_1.default.strong(config.android.platformDir)}`;
    }
    const appSrcMainAssetsWwwIndexHtmlDir = (0, path_1.join)(config.android.webDirAbs, 'index.html');
    if (!(await (0, utils_fs_1.pathExists)(appSrcMainAssetsWwwIndexHtmlDir))) {
        return `${colors_1.default.strong('index.html')} file is missing in ${colors_1.default.strong(config.android.webDirAbs)}`;
    }
    return checkAndroidManifestFile(config);
}
async function checkAndroidManifestFile(config) {
    const manifestFileName = 'AndroidManifest.xml';
    const manifestFilePath = (0, path_1.join)(config.android.srcMainDirAbs, manifestFileName);
    if (!(await (0, utils_fs_1.pathExists)(manifestFilePath))) {
        return `${colors_1.default.strong(manifestFileName)} is missing in ${colors_1.default.strong(config.android.srcMainDir)}`;
    }
    try {
        const xmlData = await (0, xml_1.readXML)(manifestFilePath);
        return checkAndroidManifestData(config, xmlData);
    }
    catch (e) {
        return e;
    }
}
async function checkAndroidManifestData(config, xmlData) {
    const manifestNode = xmlData.manifest;
    if (!manifestNode) {
        return `Missing ${colors_1.default.input('<manifest>')} XML node in ${colors_1.default.strong(config.android.srcMainDir)}`;
    }
    const applicationChildNodes = manifestNode.application;
    if (!Array.isArray(manifestNode.application)) {
        return `Missing ${colors_1.default.input('<application>')} XML node as a child node of ${colors_1.default.input('<manifest>')} in ${colors_1.default.strong(config.android.srcMainDir)}`;
    }
    let mainActivityClassPath = '';
    const mainApplicationNode = applicationChildNodes.find(applicationChildNode => {
        const activityChildNodes = applicationChildNode.activity;
        if (!Array.isArray(activityChildNodes)) {
            return false;
        }
        const mainActivityNode = activityChildNodes.find(activityChildNode => {
            const intentFilterChildNodes = activityChildNode['intent-filter'];
            if (!Array.isArray(intentFilterChildNodes)) {
                return false;
            }
            return intentFilterChildNodes.find(intentFilterChildNode => {
                const actionChildNodes = intentFilterChildNode.action;
                if (!Array.isArray(actionChildNodes)) {
                    return false;
                }
                const mainActionChildNode = actionChildNodes.find(actionChildNode => {
                    const androidName = actionChildNode.$['android:name'];
                    return androidName === 'android.intent.action.MAIN';
                });
                if (!mainActionChildNode) {
                    return false;
                }
                const categoryChildNodes = intentFilterChildNode.category;
                if (!Array.isArray(categoryChildNodes)) {
                    return false;
                }
                return categoryChildNodes.find(categoryChildNode => {
                    const androidName = categoryChildNode.$['android:name'];
                    return androidName === 'android.intent.category.LAUNCHER';
                });
            });
        });
        if (mainActivityNode) {
            mainActivityClassPath = mainActivityNode.$['android:name'];
        }
        return mainActivityNode;
    });
    if (!mainApplicationNode) {
        return `Missing main ${colors_1.default.input('<activity>')} XML node in ${colors_1.default.strong(config.android.srcMainDir)}`;
    }
    if (!mainActivityClassPath) {
        return `Missing ${colors_1.default.input('<activity android:name="">')} attribute for MainActivity class in ${colors_1.default.strong(config.android.srcMainDir)}`;
    }
    return checkPackage(config, mainActivityClassPath);
}
async function checkPackage(config, mainActivityClassPath) {
    const appSrcMainJavaDir = (0, path_1.join)(config.android.srcMainDirAbs, 'java');
    if (!(await (0, utils_fs_1.pathExists)(appSrcMainJavaDir))) {
        return `${colors_1.default.strong('java')} directory is missing in ${colors_1.default.strong(appSrcMainJavaDir)}`;
    }
    const mainActivityClassName = mainActivityClassPath.split('.').pop();
    const srcFiles = await (0, utils_fs_1.readdirp)(appSrcMainJavaDir, {
        filter: entry => !entry.stats.isDirectory() &&
            ['.java', '.kt'].includes((0, path_1.extname)(entry.path)) &&
            mainActivityClassName === (0, path_1.parse)(entry.path).name,
    });
    if (srcFiles.length == 0) {
        return `Main activity file (${mainActivityClassName}) is missing`;
    }
    return checkBuildGradle(config);
}
async function checkBuildGradle(config) {
    const fileName = 'build.gradle';
    const filePath = (0, path_1.join)(config.android.appDirAbs, fileName);
    if (!(await (0, utils_fs_1.pathExists)(filePath))) {
        return `${colors_1.default.strong(fileName)} file is missing in ${colors_1.default.strong(config.android.appDir)}`;
    }
    let fileContent = await (0, utils_fs_1.readFile)(filePath, { encoding: 'utf-8' });
    fileContent = fileContent.replace(/'|"/g, '').replace(/\s+/g, ' ');
    const searchFor = `applicationId`;
    if (fileContent.indexOf(searchFor) === -1) {
        return `${colors_1.default.strong('build.gradle')} file missing ${colors_1.default.input(`applicationId`)} config in ${filePath}`;
    }
    return null;
}
async function checkGradlew(config) {
    const fileName = 'gradlew';
    const filePath = (0, path_1.join)(config.android.platformDirAbs, fileName);
    if (!(await (0, utils_fs_1.pathExists)(filePath))) {
        return `${colors_1.default.strong(fileName)} file is missing in ${colors_1.default.strong(config.android.platformDir)}`;
    }
    return null;
}
async function checkAndroidInstalled() {
    /*
    if (!await isInstalled('android')) {
      return 'Android is not installed. For information: https://developer.android.com/studio/index.html';
    }
    */
    return null;
}
