import type { PermissionState } from '@capacitor/core';
export declare type CameraPermissionState = PermissionState | 'limited';
export declare type CameraPermissionType = 'camera' | 'photos';
export interface PermissionStatus {
    camera: CameraPermissionState;
    photos: CameraPermissionState;
}
export interface CameraPluginPermissions {
    permissions: CameraPermissionType[];
}
export interface CameraPlugin {
    /**
     * Prompt the user to pick a photo from an album, or take a new photo
     * with the camera.
     *
     * @since 1.0.0
     */
    getPhoto(options: ImageOptions): Promise<Photo>;
    /**
     * Allows the user to pick multiple pictures from the photo gallery.
     * On iOS 13 and older it only allows to pick one picture.
     *
     * @since 1.2.0
     */
    pickImages(options: GalleryImageOptions): Promise<GalleryPhotos>;
    /**
     * iOS 14+ Only: Allows the user to update their limited photo library selection.
     * On iOS 15+ returns all the limited photos after the picker dismissal.
     * On iOS 14 or if the user gave full access to the photos it returns an empty array.
     *
     * @since 4.1.0
     */
    pickLimitedLibraryPhotos(): Promise<GalleryPhotos>;
    /**
     * iOS 14+ Only: Return an array of photos selected from the limited photo library.
     *
     * @since 4.1.0
     */
    getLimitedLibraryPhotos(): Promise<GalleryPhotos>;
    /**
     * Check camera and photo album permissions
     *
     * @since 1.0.0
     */
    checkPermissions(): Promise<PermissionStatus>;
    /**
     * Request camera and photo album permissions
     *
     * @since 1.0.0
     */
    requestPermissions(permissions?: CameraPluginPermissions): Promise<PermissionStatus>;
}
export interface ImageOptions {
    /**
     * The quality of image to return as JPEG, from 0-100
     * Note: This option is only supported on Android and iOS
     *
     * @since 1.0.0
     */
    quality?: number;
    /**
     * Whether to allow the user to crop or make small edits (platform specific).
     * On iOS 14+ it's only supported for CameraSource.Camera, but not for CameraSource.Photos.
     *
     * @since 1.0.0
     */
    allowEditing?: boolean;
    /**
     * How the data should be returned. Currently, only 'Base64', 'DataUrl' or 'Uri' is supported
     *
     * @since 1.0.0
     */
    resultType: CameraResultType;
    /**
     * Whether to save the photo to the gallery.
     * If the photo was picked from the gallery, it will only be saved if edited.
     * @default: false
     *
     * @since 1.0.0
     */
    saveToGallery?: boolean;
    /**
     * The desired maximum width of the saved image. The aspect ratio is respected.
     *
     * @since 1.0.0
     */
    width?: number;
    /**
     * The desired maximum height of the saved image. The aspect ratio is respected.
     *
     * @since 1.0.0
     */
    height?: number;
    /**
     * Whether to automatically rotate the image "up" to correct for orientation
     * in portrait mode
     * @default: true
     *
     * @since 1.0.0
     */
    correctOrientation?: boolean;
    /**
     * The source to get the photo from. By default this prompts the user to select
     * either the photo album or take a photo.
     * @default: CameraSource.Prompt
     *
     * @since 1.0.0
     */
    source?: CameraSource;
    /**
     * iOS and Web only: The camera direction.
     * @default: CameraDirection.Rear
     *
     * @since 1.0.0
     */
    direction?: CameraDirection;
    /**
     * iOS only: The presentation style of the Camera.
     * @default: 'fullscreen'
     *
     * @since 1.0.0
     */
    presentationStyle?: 'fullscreen' | 'popover';
    /**
     * Web only: Whether to use the PWA Element experience or file input. The
     * default is to use PWA Elements if installed and fall back to file input.
     * To always use file input, set this to `true`.
     *
     * Learn more about PWA Elements: https://capacitorjs.com/docs/web/pwa-elements
     *
     * @since 1.0.0
     */
    webUseInput?: boolean;
    /**
     * Text value to use when displaying the prompt.
     * @default: 'Photo'
     *
     * @since 1.0.0
     *
     */
    promptLabelHeader?: string;
    /**
     * Text value to use when displaying the prompt.
     * iOS only: The label of the 'cancel' button.
     * @default: 'Cancel'
     *
     * @since 1.0.0
     */
    promptLabelCancel?: string;
    /**
     * Text value to use when displaying the prompt.
     * The label of the button to select a saved image.
     * @default: 'From Photos'
     *
     * @since 1.0.0
     */
    promptLabelPhoto?: string;
    /**
     * Text value to use when displaying the prompt.
     * The label of the button to open the camera.
     * @default: 'Take Picture'
     *
     * @since 1.0.0
     */
    promptLabelPicture?: string;
}
export interface Photo {
    /**
     * The base64 encoded string representation of the image, if using CameraResultType.Base64.
     *
     * @since 1.0.0
     */
    base64String?: string;
    /**
     * The url starting with 'data:image/jpeg;base64,' and the base64 encoded string representation of the image, if using CameraResultType.DataUrl.
     *
     * Note: On web, the file format could change depending on the browser.
     * @since 1.0.0
     */
    dataUrl?: string;
    /**
     * If using CameraResultType.Uri, the path will contain a full,
     * platform-specific file URL that can be read later using the Filesystem API.
     *
     * @since 1.0.0
     */
    path?: string;
    /**
     * webPath returns a path that can be used to set the src attribute of an image for efficient
     * loading and rendering.
     *
     * @since 1.0.0
     */
    webPath?: string;
    /**
     * Exif data, if any, retrieved from the image
     *
     * @since 1.0.0
     */
    exif?: any;
    /**
     * The format of the image, ex: jpeg, png, gif.
     *
     * iOS and Android only support jpeg.
     * Web supports jpeg, png and gif, but the exact availability may vary depending on the browser.
     * gif is only supported if `webUseInput` is set to `true` or if `source` is set to `Photos`.
     *
     * @since 1.0.0
     */
    format: string;
    /**
     * Whether if the image was saved to the gallery or not.
     *
     * On Android and iOS, saving to the gallery can fail if the user didn't
     * grant the required permissions.
     * On Web there is no gallery, so always returns false.
     *
     * @since 1.1.0
     */
    saved: boolean;
}
export interface GalleryPhotos {
    /**
     * Array of all the picked photos.
     *
     * @since 1.2.0
     */
    photos: GalleryPhoto[];
}
export interface GalleryPhoto {
    /**
     * Full, platform-specific file URL that can be read later using the Filesystem API.
     *
     * @since 1.2.0
     */
    path?: string;
    /**
     * webPath returns a path that can be used to set the src attribute of an image for efficient
     * loading and rendering.
     *
     * @since 1.2.0
     */
    webPath: string;
    /**
     * Exif data, if any, retrieved from the image
     *
     * @since 1.2.0
     */
    exif?: any;
    /**
     * The format of the image, ex: jpeg, png, gif.
     *
     * iOS and Android only support jpeg.
     * Web supports jpeg, png and gif.
     *
     * @since 1.2.0
     */
    format: string;
}
export interface GalleryImageOptions {
    /**
     * The quality of image to return as JPEG, from 0-100
     * Note: This option is only supported on Android and iOS.
     *
     * @since 1.2.0
     */
    quality?: number;
    /**
     * The desired maximum width of the saved image. The aspect ratio is respected.
     *
     * @since 1.2.0
     */
    width?: number;
    /**
     * The desired maximum height of the saved image. The aspect ratio is respected.
     *
     * @since 1.2.0
     */
    height?: number;
    /**
     * Whether to automatically rotate the image "up" to correct for orientation
     * in portrait mode
     * @default: true
     *
     * @since 1.2.0
     */
    correctOrientation?: boolean;
    /**
     * iOS only: The presentation style of the Camera.
     * @default: 'fullscreen'
     *
     * @since 1.2.0
     */
    presentationStyle?: 'fullscreen' | 'popover';
    /**
     * Maximum number of pictures the user will be able to choose.
     * Note: This option is only supported on Android 13+ and iOS.
     *
     * @default 0 (unlimited)
     *
     * @since 1.2.0
     */
    limit?: number;
}
export declare enum CameraSource {
    /**
     * Prompts the user to select either the photo album or take a photo.
     */
    Prompt = "PROMPT",
    /**
     * Take a new photo using the camera.
     */
    Camera = "CAMERA",
    /**
     * Pick an existing photo from the gallery or photo album.
     */
    Photos = "PHOTOS"
}
export declare enum CameraDirection {
    Rear = "REAR",
    Front = "FRONT"
}
export declare enum CameraResultType {
    Uri = "uri",
    Base64 = "base64",
    DataUrl = "dataUrl"
}
/**
 * @deprecated Use `Photo`.
 * @since 1.0.0
 */
export declare type CameraPhoto = Photo;
/**
 * @deprecated Use `ImageOptions`.
 * @since 1.0.0
 */
export declare type CameraOptions = ImageOptions;
