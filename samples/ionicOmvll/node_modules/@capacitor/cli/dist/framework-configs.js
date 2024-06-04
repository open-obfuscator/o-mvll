"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.detectFramework = void 0;
const FRAMEWORK_CONFIGS = [
    {
        name: 'Angular',
        isMatch: config => hasDependency(config, '@angular/cli'),
        webDir: 'dist',
        priority: 3,
    },
    {
        name: 'Create React App',
        isMatch: config => hasDependency(config, 'react-scripts'),
        webDir: 'build',
        priority: 3,
    },
    {
        name: 'Ember',
        isMatch: config => hasDependency(config, 'ember-cli'),
        webDir: 'dist',
        priority: 3,
    },
    {
        name: 'Gatsby',
        isMatch: config => hasDependency(config, 'gatsby'),
        webDir: 'public',
        priority: 2,
    },
    {
        name: 'Ionic Angular',
        isMatch: config => hasDependency(config, '@ionic/angular'),
        webDir: 'www',
        priority: 1,
    },
    {
        name: 'Ionic React',
        isMatch: config => hasDependency(config, '@ionic/react'),
        webDir: 'build',
        priority: 1,
    },
    {
        name: 'Ionic Vue',
        isMatch: config => hasDependency(config, '@ionic/vue'),
        webDir: 'public',
        priority: 1,
    },
    {
        name: 'Next',
        isMatch: config => hasDependency(config, 'next'),
        webDir: 'public',
        priority: 2,
    },
    {
        name: 'Preact',
        isMatch: config => hasDependency(config, 'preact-cli'),
        webDir: 'build',
        priority: 3,
    },
    {
        name: 'Stencil',
        isMatch: config => hasDependency(config, '@stencil/core'),
        webDir: 'www',
        priority: 3,
    },
    {
        name: 'Svelte',
        isMatch: config => hasDependency(config, 'svelte') && hasDependency(config, 'sirv-cli'),
        webDir: 'public',
        priority: 3,
    },
    {
        name: 'Vite',
        isMatch: config => hasDependency(config, 'vite'),
        webDir: 'dist',
        priority: 2,
    },
    {
        name: 'Vue',
        isMatch: config => hasDependency(config, '@vue/cli-service'),
        webDir: 'dist',
        priority: 3,
    },
];
function detectFramework(config) {
    const frameworks = FRAMEWORK_CONFIGS.filter(f => f.isMatch(config)).sort((a, b) => {
        if (a.priority < b.priority)
            return -1;
        if (a.priority > b.priority)
            return 1;
        return 0;
    });
    return frameworks[0];
}
exports.detectFramework = detectFramework;
function hasDependency(config, depName) {
    const deps = getDependencies(config);
    return deps.includes(depName);
}
function getDependencies(config) {
    var _a, _b, _c, _d;
    const deps = [];
    if ((_b = (_a = config === null || config === void 0 ? void 0 : config.app) === null || _a === void 0 ? void 0 : _a.package) === null || _b === void 0 ? void 0 : _b.dependencies) {
        deps.push(...Object.keys(config.app.package.dependencies));
    }
    if ((_d = (_c = config === null || config === void 0 ? void 0 : config.app) === null || _c === void 0 ? void 0 : _c.package) === null || _d === void 0 ? void 0 : _d.devDependencies) {
        deps.push(...Object.keys(config.app.package.devDependencies));
    }
    return deps;
}
