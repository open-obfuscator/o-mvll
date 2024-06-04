"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.resolveNode = exports.requireTS = void 0;
const utils_fs_1 = require("@ionic/utils-fs");
const fs_1 = require("fs");
const path_1 = require("path");
/**
 * @see https://github.com/ionic-team/stencil/blob/HEAD/src/compiler/sys/node-require.ts
 */
const requireTS = (ts, p) => {
    const id = (0, path_1.resolve)(p);
    delete require.cache[id];
    require.extensions['.ts'] = (module, fileName) => {
        var _a;
        let sourceText = (0, utils_fs_1.readFileSync)(fileName, 'utf8');
        if (fileName.endsWith('.ts')) {
            const tsResults = ts.transpileModule(sourceText, {
                fileName,
                compilerOptions: {
                    module: ts.ModuleKind.CommonJS,
                    moduleResolution: ts.ModuleResolutionKind.NodeJs,
                    esModuleInterop: true,
                    strict: true,
                    target: ts.ScriptTarget.ES2017,
                },
                reportDiagnostics: true,
            });
            sourceText = tsResults.outputText;
        }
        else {
            // quick hack to turn a modern es module
            // into and old school commonjs module
            sourceText = sourceText.replace(/export\s+\w+\s+(\w+)/gm, 'exports.$1');
        }
        (_a = module._compile) === null || _a === void 0 ? void 0 : _a.call(module, sourceText, fileName);
    };
    const m = require(id); // eslint-disable-line @typescript-eslint/no-var-requires
    delete require.extensions['.ts'];
    return m;
};
exports.requireTS = requireTS;
function resolveNode(root, ...pathSegments) {
    try {
        return require.resolve(pathSegments.join('/'), { paths: [root] });
    }
    catch (e) {
        const path = [root, 'node_modules', ...pathSegments].join('/');
        if ((0, fs_1.existsSync)(path)) {
            return path;
        }
        return null;
    }
}
exports.resolveNode = resolveNode;
