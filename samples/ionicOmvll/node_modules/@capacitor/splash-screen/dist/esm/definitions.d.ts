declare module '@capacitor/cli' {
    interface PluginsConfig {
        /**
         * These config values are available:
         */
        SplashScreen?: {
            /**
             * How long to show the launch splash screen when autoHide is enabled (in ms)
             *
             * @since 1.0.0
             * @default 500
             * @example 3000
             */
            launchShowDuration?: number;
            /**
             * Whether to auto hide the splash after launchShowDuration.
             *
             * @since 1.0.0
             * @default true
             * @example true
             */
            launchAutoHide?: boolean;
            /**
             * Duration for the fade out animation of the launch splash screen (in ms)
             *
             * Only available for Android, when using the Android 12 Splash Screen API.
             *
             * @since 4.2.0
             * @default 200
             * @example 3000
             */
            launchFadeOutDuration?: number;
            /**
             * Color of the background of the Splash Screen in hex format, #RRGGBB or #RRGGBBAA.
             * Doesn't work if `useDialog` is true or on launch when using the Android 12 API.
             *
             * @since 1.0.0
             * @example "#ffffffff"
             */
            backgroundColor?: string;
            /**
             * Name of the resource to be used as Splash Screen.
             *
             * Doesn't work on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.0.0
             * @default splash
             * @example "splash"
             */
            androidSplashResourceName?: string;
            /**
             * The [ImageView.ScaleType](https://developer.android.com/reference/android/widget/ImageView.ScaleType) used to scale
             * the Splash Screen image.
             * Doesn't work if `useDialog` is true or on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.0.0
             * @default FIT_XY
             * @example "CENTER_CROP"
             */
            androidScaleType?: 'CENTER' | 'CENTER_CROP' | 'CENTER_INSIDE' | 'FIT_CENTER' | 'FIT_END' | 'FIT_START' | 'FIT_XY' | 'MATRIX';
            /**
             * Show a loading spinner on the Splash Screen.
             * Doesn't work if `useDialog` is true or on launch when using the Android 12 API.
             *
             * @since 1.0.0
             * @example true
             */
            showSpinner?: boolean;
            /**
             * Style of the Android spinner.
             * Doesn't work if `useDialog` is true or on launch when using the Android 12 API.
             *
             * @since 1.0.0
             * @default large
             * @example "large"
             */
            androidSpinnerStyle?: 'horizontal' | 'small' | 'large' | 'inverse' | 'smallInverse' | 'largeInverse';
            /**
             * Style of the iOS spinner.
             * Doesn't work if `useDialog` is true.
             *
             * Only available on iOS.
             *
             * @since 1.0.0
             * @default large
             * @example "small"
             */
            iosSpinnerStyle?: 'large' | 'small';
            /**
             * Color of the spinner in hex format, #RRGGBB or #RRGGBBAA.
             * Doesn't work if `useDialog` is true or on launch when using the Android 12 API.
             *
             * @since 1.0.0
             * @example "#999999"
             */
            spinnerColor?: string;
            /**
             * Hide the status bar on the Splash Screen.
             *
             * Doesn't work on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.0.0
             * @example true
             */
            splashFullScreen?: boolean;
            /**
             * Hide the status bar and the software navigation buttons on the Splash Screen.
             *
             * Doesn't work on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.0.0
             * @example true
             */
            splashImmersive?: boolean;
            /**
             * If `useDialog` is set to true, configure the Dialog layout.
             * If `useDialog` is not set or false, use a layout instead of the ImageView.
             *
             * Doesn't work on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.1.0
             * @example "launch_screen"
             */
            layoutName?: string;
            /**
             * Use a Dialog instead of an ImageView.
             * If `layoutName` is not configured, it will use
             * a layout that uses the splash image as background.
             *
             * Doesn't work on launch when using the Android 12 API.
             *
             * Only available on Android.
             *
             * @since 1.1.0
             * @example true
             */
            useDialog?: boolean;
        };
    }
}
export interface ShowOptions {
    /**
     * Whether to auto hide the splash after showDuration
     *
     * @since 1.0.0
     */
    autoHide?: boolean;
    /**
     * How long (in ms) to fade in.
     *
     * @since 1.0.0
     * @default 200
     */
    fadeInDuration?: number;
    /**
     * How long (in ms) to fade out.
     *
     * @since 1.0.0
     * @default 200
     */
    fadeOutDuration?: number;
    /**
     * How long to show the splash screen when autoHide is enabled (in ms)
     *
     * @since 1.0.0
     * @default 3000
     */
    showDuration?: number;
}
export interface HideOptions {
    /**
     * How long (in ms) to fade out.
     *
     * On Android, if using the Android 12 Splash Screen API, it's not being used.
     * Use launchFadeOutDuration configuration option instead.
     *
     * @since 1.0.0
     * @default 200
     */
    fadeOutDuration?: number;
}
export interface SplashScreenPlugin {
    /**
     * Show the splash screen
     *
     * @since 1.0.0
     */
    show(options?: ShowOptions): Promise<void>;
    /**
     * Hide the splash screen
     *
     * @since 1.0.0
     */
    hide(options?: HideOptions): Promise<void>;
}
/**
 * @deprecated Use `ShowOptions`.
 * @since 1.0.0
 */
export declare type SplashScreenShowOptions = ShowOptions;
/**
 * @deprecated Use `HideOptions`.
 * @since 1.0.0
 */
export declare type SplashScreenHideOptions = HideOptions;
