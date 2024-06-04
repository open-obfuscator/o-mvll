package com.capacitorjs.plugins.splashscreen;

import android.widget.ImageView;
import com.getcapacitor.Logger;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;
import com.getcapacitor.util.WebColor;
import java.util.Locale;

@CapacitorPlugin(name = "SplashScreen")
public class SplashScreenPlugin extends Plugin {

    private SplashScreen splashScreen;
    private SplashScreenConfig config;

    public void load() {
        config = getSplashScreenConfig();
        splashScreen = new SplashScreen(getContext(), config);
        if (!bridge.isMinimumWebViewInstalled() && bridge.getConfig().getErrorPath() != null && !config.isLaunchAutoHide()) {
            return;
        } else {
            splashScreen.showOnLaunch(getActivity());
        }
    }

    @PluginMethod
    public void show(final PluginCall call) {
        splashScreen.show(
            getActivity(),
            getSettings(call),
            new SplashListener() {
                @Override
                public void completed() {
                    call.resolve();
                }

                @Override
                public void error() {
                    call.reject("An error occurred while showing splash");
                }
            }
        );
    }

    @PluginMethod
    public void hide(PluginCall call) {
        if (config.isUsingDialog()) {
            splashScreen.hideDialog(getActivity());
        } else {
            splashScreen.hide(getSettings(call));
        }
        call.resolve();
    }

    @Override
    protected void handleOnPause() {
        splashScreen.onPause();
    }

    @Override
    protected void handleOnDestroy() {
        splashScreen.onDestroy();
    }

    private SplashScreenSettings getSettings(PluginCall call) {
        SplashScreenSettings settings = new SplashScreenSettings();
        if (call.getInt("showDuration") != null) {
            settings.setShowDuration(call.getInt("showDuration"));
        }
        if (call.getInt("fadeInDuration") != null) {
            settings.setFadeInDuration(call.getInt("fadeInDuration"));
        }
        if (call.getInt("fadeOutDuration") != null) {
            settings.setFadeOutDuration(call.getInt("fadeOutDuration"));
        }
        if (call.getBoolean("autoHide") != null) {
            settings.setAutoHide(call.getBoolean("autoHide"));
        }
        return settings;
    }

    private SplashScreenConfig getSplashScreenConfig() {
        SplashScreenConfig config = new SplashScreenConfig();
        String backgroundColor = getConfig().getString("backgroundColor");
        if (backgroundColor != null) {
            try {
                config.setBackgroundColor(WebColor.parseColor(backgroundColor));
            } catch (IllegalArgumentException ex) {
                Logger.debug("Background color not applied");
            }
        }
        Integer duration = getConfig().getInt("launchShowDuration", config.getLaunchShowDuration());
        config.setLaunchShowDuration(duration);
        Integer fadeOutDuration = getConfig().getInt("launchFadeOutDuration", config.getLaunchFadeOutDuration());
        config.setLaunchFadeOutDuration(fadeOutDuration);
        Boolean autohide = getConfig().getBoolean("launchAutoHide", config.isLaunchAutoHide());
        config.setLaunchAutoHide(autohide);
        if (getConfig().getString("androidSplashResourceName") != null) {
            config.setResourceName(getConfig().getString("androidSplashResourceName"));
        }
        Boolean immersive = getConfig().getBoolean("splashImmersive", config.isImmersive());
        config.setImmersive(immersive);

        Boolean fullScreen = getConfig().getBoolean("splashFullScreen", config.isFullScreen());
        config.setFullScreen(fullScreen);

        String spinnerStyle = getConfig().getString("androidSpinnerStyle");
        if (spinnerStyle != null) {
            int spinnerBarStyle = android.R.attr.progressBarStyleLarge;

            switch (spinnerStyle.toLowerCase(Locale.ROOT)) {
                case "horizontal":
                    spinnerBarStyle = android.R.attr.progressBarStyleHorizontal;
                    break;
                case "small":
                    spinnerBarStyle = android.R.attr.progressBarStyleSmall;
                    break;
                case "large":
                    spinnerBarStyle = android.R.attr.progressBarStyleLarge;
                    break;
                case "inverse":
                    spinnerBarStyle = android.R.attr.progressBarStyleInverse;
                    break;
                case "smallinverse":
                    spinnerBarStyle = android.R.attr.progressBarStyleSmallInverse;
                    break;
                case "largeinverse":
                    spinnerBarStyle = android.R.attr.progressBarStyleLargeInverse;
                    break;
            }
            config.setSpinnerStyle(spinnerBarStyle);
        }
        String spinnerColor = getConfig().getString("spinnerColor");
        if (spinnerColor != null) {
            try {
                config.setSpinnerColor(WebColor.parseColor(spinnerColor));
            } catch (IllegalArgumentException ex) {
                Logger.debug("Spinner color not applied");
            }
        }
        String scaleTypeName = getConfig().getString("androidScaleType");
        if (scaleTypeName != null) {
            ImageView.ScaleType scaleType = null;
            try {
                scaleType = ImageView.ScaleType.valueOf(scaleTypeName);
            } catch (IllegalArgumentException ex) {
                scaleType = ImageView.ScaleType.FIT_XY;
            }
            config.setScaleType(scaleType);
        }

        Boolean showSpinner = getConfig().getBoolean("showSpinner", config.isShowSpinner());
        config.setShowSpinner(showSpinner);

        Boolean useDialog = getConfig().getBoolean("useDialog", config.isUsingDialog());
        config.setUsingDialog(useDialog);

        if (getConfig().getString("layoutName") != null) {
            config.setLayoutName(getConfig().getString("layoutName"));
        }

        return config;
    }
}
