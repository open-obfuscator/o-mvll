"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.inlineSourceMaps = void 0;
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const log_1 = require("../log");
function walkDirectory(dirPath) {
    const files = (0, utils_fs_1.readdirSync)(dirPath);
    files.forEach(file => {
        const targetFile = (0, path_1.join)(dirPath, file);
        if ((0, utils_fs_1.existsSync)(targetFile) && (0, utils_fs_1.lstatSync)(targetFile).isDirectory()) {
            walkDirectory(targetFile);
        }
        else {
            const mapFile = (0, path_1.join)(dirPath, `${file}.map`);
            if ((0, path_1.extname)(file) === '.js' && (0, utils_fs_1.existsSync)(mapFile)) {
                const bufMap = (0, utils_fs_1.readFileSync)(mapFile).toString('base64');
                const bufFile = (0, utils_fs_1.readFileSync)(targetFile, 'utf8');
                const result = bufFile.replace(`sourceMappingURL=${file}.map`, 'sourceMappingURL=data:application/json;charset=utf-8;base64,' +
                    bufMap);
                (0, utils_fs_1.writeFileSync)(targetFile, result);
                (0, utils_fs_1.unlinkSync)(mapFile);
            }
        }
    });
}
async function inlineSourceMaps(config, platformName) {
    let buildDir = '';
    if (platformName == config.ios.name) {
        buildDir = await config.ios.webDirAbs;
    }
    if (platformName == config.android.name) {
        buildDir = await config.android.webDirAbs;
    }
    if (buildDir) {
        log_1.logger.info('Inlining sourcemaps');
        walkDirectory(buildDir);
    }
}
exports.inlineSourceMaps = inlineSourceMaps;
