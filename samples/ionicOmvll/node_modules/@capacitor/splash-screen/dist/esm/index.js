import { registerPlugin } from '@capacitor/core';
const SplashScreen = registerPlugin('SplashScreen', {
    web: () => import('./web').then(m => new m.SplashScreenWeb()),
});
export * from './definitions';
export { SplashScreen };
//# sourceMappingURL=index.js.map