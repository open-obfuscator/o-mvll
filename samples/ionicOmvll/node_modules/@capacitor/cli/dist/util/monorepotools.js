"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.isNXMonorepo = exports.isMonorepo = exports.findPackageRelativePathInMonorepo = exports.findPackagePath = exports.findNXMonorepoRoot = exports.findMonorepoRoot = void 0;
const node_fs_1 = require("node:fs");
const node_path_1 = require("node:path");
/**
 * Finds the monorepo root from the given path.
 * @param currentPath - The current path to start searching from.
 * @returns The path to the monorepo root.
 * @throws An error if the monorepo root is not found.
 */
function findMonorepoRoot(currentPath) {
    const packageJsonPath = (0, node_path_1.join)(currentPath, 'package.json');
    const pnpmWorkspacePath = (0, node_path_1.join)(currentPath, 'pnpm-workspace.yaml');
    if ((0, node_fs_1.existsSync)(pnpmWorkspacePath) ||
        ((0, node_fs_1.existsSync)(packageJsonPath) &&
            JSON.parse((0, node_fs_1.readFileSync)(packageJsonPath, 'utf-8')).workspaces)) {
        return currentPath;
    }
    const parentPath = (0, node_path_1.dirname)(currentPath);
    if (parentPath === currentPath) {
        throw new Error('Monorepo root not found');
    }
    return findMonorepoRoot(parentPath);
}
exports.findMonorepoRoot = findMonorepoRoot;
/**
 * Finds the NX monorepo root from the given path.
 * @param currentPath - The current path to start searching from.
 * @returns The path to the monorepo root.
 * @throws An error if the monorepo root is not found.
 */
function findNXMonorepoRoot(currentPath) {
    const nxJsonPath = (0, node_path_1.join)(currentPath, 'nx.json');
    if ((0, node_fs_1.existsSync)(nxJsonPath)) {
        return currentPath;
    }
    const parentPath = (0, node_path_1.dirname)(currentPath);
    if (parentPath === currentPath) {
        throw new Error('Monorepo root not found');
    }
    return findNXMonorepoRoot(parentPath);
}
exports.findNXMonorepoRoot = findNXMonorepoRoot;
/**
 * Finds the path to a package within the node_modules folder,
 * searching up the directory hierarchy until the last possible directory is reached.
 * @param packageName - The name of the package to find.
 * @param currentPath - The current path to start searching from.
 * @param lastPossibleDirectory - The last possible directory to search for the package.
 * @returns The path to the package, or null if not found.
 */
function findPackagePath(packageName, currentPath, lastPossibleDirectory) {
    const nodeModulesPath = (0, node_path_1.join)(currentPath, 'node_modules', packageName);
    if ((0, node_fs_1.existsSync)(nodeModulesPath)) {
        return nodeModulesPath;
    }
    if (currentPath === lastPossibleDirectory) {
        return null;
    }
    const parentPath = (0, node_path_1.dirname)(currentPath);
    return findPackagePath(packageName, parentPath, lastPossibleDirectory);
}
exports.findPackagePath = findPackagePath;
/**
 * Finds the relative path to a package from the current directory,
 * using the monorepo root as the last possible directory.
 * @param packageName - The name of the package to find.
 * @param currentPath - The current path to start searching from.
 * @returns The relative path to the package, or null if not found.
 */
function findPackageRelativePathInMonorepo(packageName, currentPath) {
    const monorepoRoot = findMonorepoRoot(currentPath);
    const packagePath = findPackagePath(packageName, currentPath, monorepoRoot);
    if (packagePath) {
        return (0, node_path_1.relative)(currentPath, packagePath);
    }
    return null;
}
exports.findPackageRelativePathInMonorepo = findPackageRelativePathInMonorepo;
/**
 * Detects if the current directory is part of a monorepo (npm, yarn, pnpm).
 * @param currentPath - The current path to start searching from.
 * @returns True if the current directory is part of a monorepo, false otherwise.
 */
function isMonorepo(currentPath) {
    try {
        findMonorepoRoot(currentPath);
        return true;
    }
    catch (error) {
        return false;
    }
}
exports.isMonorepo = isMonorepo;
/**
 * Detects if the current directory is part of a nx integrated monorepo.
 * @param currentPath - The current path to start searching from.
 * @returns True if the current directory is part of a monorepo, false otherwise.
 */
function isNXMonorepo(currentPath) {
    try {
        findNXMonorepoRoot(currentPath);
        return true;
    }
    catch (error) {
        return false;
    }
}
exports.isNXMonorepo = isNXMonorepo;
