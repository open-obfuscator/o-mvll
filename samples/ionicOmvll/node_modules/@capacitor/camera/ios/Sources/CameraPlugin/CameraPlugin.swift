import Foundation
import Capacitor
import Photos
import PhotosUI

@objc(CAPCameraPlugin)
public class CameraPlugin: CAPPlugin, CAPBridgedPlugin {
    public let identifier = "CAPCameraPlugin"
    public let jsName = "Camera"
    public let pluginMethods: [CAPPluginMethod] = [
        CAPPluginMethod(name: "getPhoto", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "pickImages", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "checkPermissions", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "requestPermissions", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "pickLimitedLibraryPhotos", returnType: CAPPluginReturnPromise),
        CAPPluginMethod(name: "getLimitedLibraryPhotos", returnType: CAPPluginReturnPromise)
    ]
    private var call: CAPPluginCall?
    private var settings = CameraSettings()
    private let defaultSource = CameraSource.prompt
    private let defaultDirection = CameraDirection.rear
    private var multiple = false

    private var imageCounter = 0

    @objc override public func checkPermissions(_ call: CAPPluginCall) {
        var result: [String: Any] = [:]
        for permission in CameraPermissionType.allCases {
            let state: String
            switch permission {
            case .camera:
                state = AVCaptureDevice.authorizationStatus(for: .video).authorizationState
            case .photos:
                if #available(iOS 14, *) {
                    state = PHPhotoLibrary.authorizationStatus(for: .readWrite).authorizationState
                } else {
                    state = PHPhotoLibrary.authorizationStatus().authorizationState
                }
            }
            result[permission.rawValue] = state
        }
        call.resolve(result)
    }

    @objc override public func requestPermissions(_ call: CAPPluginCall) {
        // get the list of desired types, if passed
        let typeList = call.getArray("permissions", String.self)?.compactMap({ (type) -> CameraPermissionType? in
            return CameraPermissionType(rawValue: type)
        }) ?? []
        // otherwise check everything
        let permissions: [CameraPermissionType] = (typeList.count > 0) ? typeList : CameraPermissionType.allCases
        // request the permissions
        let group = DispatchGroup()
        for permission in permissions {
            switch permission {
            case .camera:
                group.enter()
                AVCaptureDevice.requestAccess(for: .video) { _ in
                    group.leave()
                }
            case .photos:
                group.enter()
                if #available(iOS 14, *) {
                    PHPhotoLibrary.requestAuthorization(for: .readWrite) { (_) in
                        group.leave()
                    }
                } else {
                    PHPhotoLibrary.requestAuthorization({ (_) in
                        group.leave()
                    })
                }
            }
        }
        group.notify(queue: DispatchQueue.main) { [weak self] in
            self?.checkPermissions(call)
        }
    }

    @objc func pickLimitedLibraryPhotos(_ call: CAPPluginCall) {
        if #available(iOS 14, *) {
            PHPhotoLibrary.requestAuthorization(for: .readWrite) { (granted) in
                if granted == .limited {
                    if let viewController = self.bridge?.viewController {
                        if #available(iOS 15, *) {
                            PHPhotoLibrary.shared().presentLimitedLibraryPicker(from: viewController) { _ in
                                self.getLimitedLibraryPhotos(call)
                            }
                        } else {
                            PHPhotoLibrary.shared().presentLimitedLibraryPicker(from: viewController)
                            call.resolve([
                                "photos": []
                            ])
                        }
                    }
                } else {
                    call.resolve([
                        "photos": []
                    ])
                }
            }
        } else {
            call.unavailable("Not available on iOS 13")
        }
    }

    @objc func getLimitedLibraryPhotos(_ call: CAPPluginCall) {
        if #available(iOS 14, *) {
            PHPhotoLibrary.requestAuthorization(for: .readWrite) { (granted) in
                if granted == .limited {

                    self.call = call

                    DispatchQueue.global(qos: .utility).async {
                        let assets = PHAsset.fetchAssets(with: .image, options: nil)
                        var processedImages: [ProcessedImage] = []

                        let imageManager = PHImageManager.default()
                        let options = PHImageRequestOptions()
                        options.deliveryMode = .highQualityFormat

                        let group = DispatchGroup()
                        if assets.count > 0 {
                            for index in 0...(assets.count - 1) {
                                let asset = assets.object(at: index)
                                let fullSize = CGSize(width: asset.pixelWidth, height: asset.pixelHeight)

                                group.enter()
                                imageManager.requestImage(for: asset, targetSize: fullSize, contentMode: .default, options: options) { image, _ in
                                    guard let image = image else {
                                        group.leave()
                                        return
                                    }
                                    processedImages.append(self.processedImage(from: image, with: asset.imageData))
                                    group.leave()
                                }
                            }
                        }

                        group.notify(queue: .global(qos: .utility)) { [weak self] in
                            self?.returnImages(processedImages)
                        }
                    }
                } else {
                    call.resolve([
                        "photos": []
                    ])
                }
            }
        } else {
            call.unavailable("Not available on iOS 13")
        }
    }

    @objc func getPhoto(_ call: CAPPluginCall) {
        self.multiple = false
        self.call = call
        self.settings = cameraSettings(from: call)

        // Make sure they have all the necessary info.plist settings
        if let missingUsageDescription = checkUsageDescriptions() {
            CAPLog.print("⚡️ ", self.pluginId, "-", missingUsageDescription)
            call.reject(missingUsageDescription)
            return
        }

        DispatchQueue.main.async {
            switch self.settings.source {
            case .prompt:
                self.showPrompt()
            case .camera:
                self.showCamera()
            case .photos:
                self.showPhotos()
            }
        }
    }

    @objc func pickImages(_ call: CAPPluginCall) {
        self.multiple = true
        self.call = call
        self.settings = cameraSettings(from: call)
        DispatchQueue.main.async {
            self.showPhotos()
        }
    }

    private func checkUsageDescriptions() -> String? {
        if let dict = Bundle.main.infoDictionary {
            for key in CameraPropertyListKeys.allCases where dict[key.rawValue] == nil {
                return key.missingMessage
            }
        }
        return nil
    }

    private func cameraSettings(from call: CAPPluginCall) -> CameraSettings {
        var settings = CameraSettings()
        settings.jpegQuality = min(abs(CGFloat(call.getFloat("quality") ?? 100.0)) / 100.0, 1.0)
        settings.allowEditing = call.getBool("allowEditing") ?? false
        settings.source = CameraSource(rawValue: call.getString("source") ?? defaultSource.rawValue) ?? defaultSource
        settings.direction = CameraDirection(rawValue: call.getString("direction") ?? defaultDirection.rawValue) ?? defaultDirection
        if let typeString = call.getString("resultType"), let type = CameraResultType(rawValue: typeString) {
            settings.resultType = type
        }
        settings.saveToGallery = call.getBool("saveToGallery") ?? false

        // Get the new image dimensions if provided
        settings.width = CGFloat(call.getInt("width") ?? 0)
        settings.height = CGFloat(call.getInt("height") ?? 0)
        if settings.width > 0 || settings.height > 0 {
            // We resize only if a dimension was provided
            settings.shouldResize = true
        }
        settings.shouldCorrectOrientation = call.getBool("correctOrientation") ?? true
        settings.userPromptText = CameraPromptText(title: call.getString("promptLabelHeader"),
                                                   photoAction: call.getString("promptLabelPhoto"),
                                                   cameraAction: call.getString("promptLabelPicture"),
                                                   cancelAction: call.getString("promptLabelCancel"))
        if let styleString = call.getString("presentationStyle"), styleString == "popover" {
            settings.presentationStyle = .popover
        } else {
            settings.presentationStyle = .fullScreen
        }

        return settings
    }
}

// public delegate methods
extension CameraPlugin: UIImagePickerControllerDelegate, UINavigationControllerDelegate, UIPopoverPresentationControllerDelegate {
    public func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
        picker.dismiss(animated: true)
        self.call?.reject("User cancelled photos app")
    }

    public func popoverPresentationControllerDidDismissPopover(_ popoverPresentationController: UIPopoverPresentationController) {
        self.call?.reject("User cancelled photos app")
    }

    public func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
        self.call?.reject("User cancelled photos app")
    }

    public func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey: Any]) {
        picker.dismiss(animated: true) {
            if let processedImage = self.processImage(from: info) {
                self.returnProcessedImage(processedImage)
            } else {
                self.call?.reject("Error processing image")
            }
        }
    }
}

@available(iOS 14, *)
extension CameraPlugin: PHPickerViewControllerDelegate {
    public func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
        picker.dismiss(animated: true, completion: nil)
        guard let result = results.first else {
            self.call?.reject("User cancelled photos app")
            return
        }
        if multiple {
            var images: [ProcessedImage] = []
            var processedCount = 0
            for img in results {
                guard img.itemProvider.canLoadObject(ofClass: UIImage.self) else {
                    self.call?.reject("Error loading image")
                    return
                }
                // extract the image
                img.itemProvider.loadObject(ofClass: UIImage.self) { [weak self] (reading, _) in
                    if let image = reading as? UIImage {
                        var asset: PHAsset?
                        if let assetId = img.assetIdentifier {
                            asset = PHAsset.fetchAssets(withLocalIdentifiers: [assetId], options: nil).firstObject
                        }
                        if let processedImage = self?.processedImage(from: image, with: asset?.imageData) {
                            images.append(processedImage)
                        }
                        processedCount += 1
                        if processedCount == results.count {
                            self?.returnImages(images)
                        }
                    } else {
                        self?.call?.reject("Error loading image")
                    }
                }
            }

        } else {
            guard result.itemProvider.canLoadObject(ofClass: UIImage.self) else {
                self.call?.reject("Error loading image")
                return
            }
            // extract the image
            result.itemProvider.loadObject(ofClass: UIImage.self) { [weak self] (reading, _) in
                if let image = reading as? UIImage {
                    var asset: PHAsset?
                    if let assetId = result.assetIdentifier {
                        asset = PHAsset.fetchAssets(withLocalIdentifiers: [assetId], options: nil).firstObject
                    }
                    if var processedImage = self?.processedImage(from: image, with: asset?.imageData) {
                        processedImage.flags = .gallery
                        self?.returnProcessedImage(processedImage)
                        return
                    }
                }
                self?.call?.reject("Error loading image")
            }
        }
    }
}

private extension CameraPlugin {
    func returnImage(_ processedImage: ProcessedImage, isSaved: Bool) {
        guard let jpeg = processedImage.generateJPEG(with: settings.jpegQuality) else {
            self.call?.reject("Unable to convert image to jpeg")
            return
        }

        if settings.resultType == CameraResultType.uri || multiple {
            guard let fileURL = try? saveTemporaryImage(jpeg),
                  let webURL = bridge?.portablePath(fromLocalURL: fileURL) else {
                call?.reject("Unable to get portable path to file")
                return
            }
            if self.multiple {
                call?.resolve([
                    "photos": [[
                        "path": fileURL.absoluteString,
                        "exif": processedImage.exifData,
                        "webPath": webURL.absoluteString,
                        "format": "jpeg"
                    ]]
                ])
                return
            }
            call?.resolve([
                "path": fileURL.absoluteString,
                "exif": processedImage.exifData,
                "webPath": webURL.absoluteString,
                "format": "jpeg",
                "saved": isSaved
            ])
        } else if settings.resultType == CameraResultType.base64 {
            self.call?.resolve([
                "base64String": jpeg.base64EncodedString(),
                "exif": processedImage.exifData,
                "format": "jpeg",
                "saved": isSaved
            ])
        } else if settings.resultType == CameraResultType.dataURL {
            call?.resolve([
                "dataUrl": "data:image/jpeg;base64," + jpeg.base64EncodedString(),
                "exif": processedImage.exifData,
                "format": "jpeg",
                "saved": isSaved
            ])
        }
    }

    func returnImages(_ processedImages: [ProcessedImage]) {
        var photos: [PluginCallResultData] = []
        for processedImage in processedImages {
            guard let jpeg = processedImage.generateJPEG(with: settings.jpegQuality) else {
                self.call?.reject("Unable to convert image to jpeg")
                return
            }

            guard let fileURL = try? saveTemporaryImage(jpeg),
                  let webURL = bridge?.portablePath(fromLocalURL: fileURL) else {
                call?.reject("Unable to get portable path to file")
                return
            }

            photos.append([
                "path": fileURL.absoluteString,
                "exif": processedImage.exifData,
                "webPath": webURL.absoluteString,
                "format": "jpeg"
            ])
        }
        call?.resolve([
            "photos": photos
        ])
    }

    func returnProcessedImage(_ processedImage: ProcessedImage) {
        // conditionally save the image
        if settings.saveToGallery && (processedImage.flags.contains(.edited) == true || processedImage.flags.contains(.gallery) == false) {
            _ = ImageSaver(image: processedImage.image) { error in
                var isSaved = false
                if error == nil {
                    isSaved = true
                }
                self.returnImage(processedImage, isSaved: isSaved)
            }
        } else {
            self.returnImage(processedImage, isSaved: false)
        }
    }

    func showPrompt() {
        // Build the action sheet
        let alert = UIAlertController(title: settings.userPromptText.title, message: nil, preferredStyle: UIAlertController.Style.actionSheet)
        alert.addAction(UIAlertAction(title: settings.userPromptText.photoAction, style: .default, handler: { [weak self] (_: UIAlertAction) in
            self?.showPhotos()
        }))

        alert.addAction(UIAlertAction(title: settings.userPromptText.cameraAction, style: .default, handler: { [weak self] (_: UIAlertAction) in
            self?.showCamera()
        }))

        alert.addAction(UIAlertAction(title: settings.userPromptText.cancelAction, style: .cancel, handler: { [weak self] (_: UIAlertAction) in
            self?.call?.reject("User cancelled photos app")
        }))
        self.setCenteredPopover(alert)
        self.bridge?.viewController?.present(alert, animated: true, completion: nil)
    }

    func showCamera() {
        // check if we have a camera
        if (bridge?.isSimEnvironment ?? false) || !UIImagePickerController.isSourceTypeAvailable(UIImagePickerController.SourceType.camera) {
            CAPLog.print("⚡️ ", self.pluginId, "-", "Camera not available in simulator")
            call?.reject("Camera not available while running in Simulator")
            return
        }
        // check for permission
        let authStatus = AVCaptureDevice.authorizationStatus(for: .video)
        if authStatus == .restricted || authStatus == .denied {
            call?.reject("User denied access to camera")
            return
        }
        // we either already have permission or can prompt
        AVCaptureDevice.requestAccess(for: .video) { [weak self] granted in
            if granted {
                DispatchQueue.main.async {
                    self?.presentCameraPicker()
                }
            } else {
                self?.call?.reject("User denied access to camera")
            }
        }
    }

    func showPhotos() {
        // check for permission
        let authStatus = PHPhotoLibrary.authorizationStatus()
        if authStatus == .restricted || authStatus == .denied {
            call?.reject("User denied access to photos")
            return
        }
        // we either already have permission or can prompt
        if authStatus == .authorized {
            presentSystemAppropriateImagePicker()
        } else {
            PHPhotoLibrary.requestAuthorization({ [weak self] (status) in
                if status == PHAuthorizationStatus.authorized {
                    DispatchQueue.main.async { [weak self] in
                        self?.presentSystemAppropriateImagePicker()
                    }
                } else {
                    self?.call?.reject("User denied access to photos")
                }
            })
        }
    }

    func presentCameraPicker() {
        let picker = UIImagePickerController()
        picker.delegate = self
        picker.allowsEditing = self.settings.allowEditing
        // select the input
        picker.sourceType = .camera
        if settings.direction == .rear, UIImagePickerController.isCameraDeviceAvailable(.rear) {
            picker.cameraDevice = .rear
        } else if settings.direction == .front, UIImagePickerController.isCameraDeviceAvailable(.front) {
            picker.cameraDevice = .front
        }
        // present
        picker.modalPresentationStyle = settings.presentationStyle
        if settings.presentationStyle == .popover {
            picker.popoverPresentationController?.delegate = self
            setCenteredPopover(picker)
        }
        bridge?.viewController?.present(picker, animated: true, completion: nil)
    }

    func presentSystemAppropriateImagePicker() {
        if #available(iOS 14, *) {
            presentPhotoPicker()
        } else {
            presentImagePicker()
        }
    }

    func presentImagePicker() {
        let picker = UIImagePickerController()
        picker.delegate = self
        picker.allowsEditing = self.settings.allowEditing
        // select the input
        picker.sourceType = .photoLibrary
        // present
        picker.modalPresentationStyle = settings.presentationStyle
        if settings.presentationStyle == .popover {
            picker.popoverPresentationController?.delegate = self
            setCenteredPopover(picker)
        }
        bridge?.viewController?.present(picker, animated: true, completion: nil)
    }

    @available(iOS 14, *)
    func presentPhotoPicker() {
        var configuration = PHPickerConfiguration(photoLibrary: PHPhotoLibrary.shared())
        configuration.selectionLimit = self.multiple ? (self.call?.getInt("limit") ?? 0) : 1
        configuration.filter = .images
        let picker = PHPickerViewController(configuration: configuration)
        picker.delegate = self
        // present
        picker.modalPresentationStyle = settings.presentationStyle
        if settings.presentationStyle == .popover {
            picker.popoverPresentationController?.delegate = self
            setCenteredPopover(picker)
        }
        bridge?.viewController?.present(picker, animated: true, completion: nil)
    }

    func saveTemporaryImage(_ data: Data) throws -> URL {
        var url: URL
        repeat {
            imageCounter += 1
            url = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("photo-\(imageCounter).jpg")
        } while FileManager.default.fileExists(atPath: url.path)

        try data.write(to: url, options: .atomic)
        return url
    }

    func processImage(from info: [UIImagePickerController.InfoKey: Any]) -> ProcessedImage? {
        var selectedImage: UIImage?
        var flags: PhotoFlags = []
        // get the image
        if let edited = info[UIImagePickerController.InfoKey.editedImage] as? UIImage {
            selectedImage = edited // use the edited version
            flags = flags.union([.edited])
        } else if let original = info[UIImagePickerController.InfoKey.originalImage] as? UIImage {
            selectedImage = original // use the original version
        }
        guard let image = selectedImage else {
            return nil
        }
        var metadata: [String: Any] = [:]
        // get the image's metadata from the picker or from the photo album
        if let photoMetadata = info[UIImagePickerController.InfoKey.mediaMetadata] as? [String: Any] {
            metadata = photoMetadata
        } else {
            flags = flags.union([.gallery])
        }
        if let asset = info[UIImagePickerController.InfoKey.phAsset] as? PHAsset {
            metadata = asset.imageData
        }
        // get the result
        var result = processedImage(from: image, with: metadata)
        result.flags = flags
        return result
    }

    func processedImage(from image: UIImage, with metadata: [String: Any]?) -> ProcessedImage {
        var result = ProcessedImage(image: image, metadata: metadata ?? [:])
        // resizing the image only makes sense if we have real values to which to constrain it
        if settings.shouldResize, settings.width > 0 || settings.height > 0 {
            result.image = result.image.reformat(to: CGSize(width: settings.width, height: settings.height))
            result.overwriteMetadataOrientation(to: 1)
        } else if settings.shouldCorrectOrientation {
            // resizing implicitly reformats the image so this is only needed if we aren't resizing
            result.image = result.image.reformat()
            result.overwriteMetadataOrientation(to: 1)
        }
        return result
    }
}
