"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.buildAndroid = void 0;
const tslib_1 = require("tslib");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const log_1 = require("../log");
const subprocess_1 = require("../util/subprocess");
async function buildAndroid(config, buildOptions) {
    var _a, _b;
    const releaseType = (_a = buildOptions.androidreleasetype) !== null && _a !== void 0 ? _a : 'AAB';
    const releaseTypeIsAAB = releaseType === 'AAB';
    const flavor = (_b = buildOptions.flavor) !== null && _b !== void 0 ? _b : '';
    const arg = releaseTypeIsAAB
        ? `:app:bundle${flavor}Release`
        : `assemble${flavor}Release`;
    const gradleArgs = [arg];
    try {
        await (0, common_1.runTask)('Running Gradle build', async () => (0, subprocess_1.runCommand)('./gradlew', gradleArgs, {
            cwd: config.android.platformDirAbs,
        }));
    }
    catch (e) {
        if (e.includes('EACCES')) {
            throw `gradlew file does not have executable permissions. This can happen if the Android platform was added on a Windows machine. Please run ${colors_1.default.strong(`chmod +x ./${config.android.platformDir}/gradlew`)} and try again.`;
        }
        else {
            throw e;
        }
    }
    const releaseDir = releaseTypeIsAAB
        ? flavor !== ''
            ? `${flavor}Release`
            : 'release'
        : flavor !== ''
            ? (0, path_1.join)(flavor, 'release')
            : 'release';
    const releasePath = (0, path_1.join)(config.android.appDirAbs, 'build', 'outputs', releaseTypeIsAAB ? 'bundle' : 'apk', releaseDir);
    const unsignedReleaseName = `app${flavor !== '' ? `-${flavor}` : ''}-release${releaseTypeIsAAB ? '' : '-unsigned'}.${releaseType.toLowerCase()}`;
    const signedReleaseName = unsignedReleaseName.replace(`-release${releaseTypeIsAAB ? '' : '-unsigned'}.${releaseType.toLowerCase()}`, `-release-signed.${releaseType.toLowerCase()}`);
    if (buildOptions.signingtype == 'jarsigner') {
        await signWithJarSigner(config, buildOptions, releasePath, signedReleaseName, unsignedReleaseName);
    }
    else {
        await signWithApkSigner(config, buildOptions, releasePath, signedReleaseName, unsignedReleaseName);
    }
    (0, log_1.logSuccess)(`Successfully generated ${signedReleaseName} at: ${releasePath}`);
}
exports.buildAndroid = buildAndroid;
async function signWithApkSigner(config, buildOptions, releasePath, signedReleaseName, unsignedReleaseName) {
    if (!buildOptions.keystorepath || !buildOptions.keystorepass) {
        throw 'Missing options. Please supply all options for android signing. (Keystore Path, Keystore Password)';
    }
    const signingArgs = [
        'sign',
        '--ks',
        buildOptions.keystorepath,
        '--ks-pass',
        `pass:${buildOptions.keystorepass}`,
        '--in',
        `${(0, path_1.join)(releasePath, unsignedReleaseName)}`,
        '--out',
        `${(0, path_1.join)(releasePath, signedReleaseName)}`,
    ];
    if (buildOptions.keystorealias) {
        signingArgs.push('--ks-key-alias', buildOptions.keystorealias);
    }
    if (buildOptions.keystorealiaspass) {
        signingArgs.push('--key-pass', `pass:${buildOptions.keystorealiaspass}`);
    }
    await (0, common_1.runTask)('Signing Release', async () => {
        await (0, subprocess_1.runCommand)('apksigner', signingArgs, {
            cwd: config.android.platformDirAbs,
        });
    });
}
async function signWithJarSigner(config, buildOptions, releasePath, signedReleaseName, unsignedReleaseName) {
    if (!buildOptions.keystorepath ||
        !buildOptions.keystorealias ||
        !buildOptions.keystorealiaspass ||
        !buildOptions.keystorepass) {
        throw 'Missing options. Please supply all options for android signing. (Keystore Path, Keystore Password, Keystore Key Alias, Keystore Key Password)';
    }
    const signingArgs = [
        '-sigalg',
        'SHA1withRSA',
        '-digestalg',
        'SHA1',
        '-keystore',
        buildOptions.keystorepath,
        '-keypass',
        buildOptions.keystorealiaspass,
        '-storepass',
        buildOptions.keystorepass,
        `-signedjar`,
        `${(0, path_1.join)(releasePath, signedReleaseName)}`,
        `${(0, path_1.join)(releasePath, unsignedReleaseName)}`,
        buildOptions.keystorealias,
    ];
    await (0, common_1.runTask)('Signing Release', async () => {
        await (0, subprocess_1.runCommand)('jarsigner', signingArgs, {
            cwd: config.android.platformDirAbs,
        });
    });
}
