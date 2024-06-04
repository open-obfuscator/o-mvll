import { WebPlugin } from '@capacitor/core';
import type { HideOptions, ShowOptions, SplashScreenPlugin } from './definitions';
export declare class SplashScreenWeb extends WebPlugin implements SplashScreenPlugin {
    show(_options?: ShowOptions): Promise<void>;
    hide(_options?: HideOptions): Promise<void>;
}
