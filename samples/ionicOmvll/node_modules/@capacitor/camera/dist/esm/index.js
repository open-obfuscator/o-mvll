import { registerPlugin } from '@capacitor/core';
import { CameraWeb } from './web';
const Camera = registerPlugin('Camera', {
    web: () => new CameraWeb(),
});
export * from './definitions';
export { Camera };
//# sourceMappingURL=index.js.map