"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deleteFolderRecursive = exports.convertToUnixPath = void 0;
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const convertToUnixPath = (path) => {
    return path.replace(/\\/g, '/');
};
exports.convertToUnixPath = convertToUnixPath;
const deleteFolderRecursive = (directoryPath) => {
    if ((0, utils_fs_1.existsSync)(directoryPath)) {
        (0, utils_fs_1.readdirSync)(directoryPath).forEach(file => {
            const curPath = (0, path_1.join)(directoryPath, file);
            if ((0, utils_fs_1.lstatSync)(curPath).isDirectory()) {
                (0, exports.deleteFolderRecursive)(curPath);
            }
            else {
                (0, utils_fs_1.unlinkSync)(curPath);
            }
        });
        (0, utils_fs_1.rmdirSync)(directoryPath);
    }
};
exports.deleteFolderRecursive = deleteFolderRecursive;
