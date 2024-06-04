package com.capacitorjs.plugins.splashscreen;

import android.widget.ImageView.ScaleType;

public class SplashScreenConfig {

    private Integer backgroundColor;
    private Integer spinnerStyle;
    private Integer spinnerColor;
    private boolean showSpinner = false;
    private Integer launchShowDuration = 500;
    private boolean launchAutoHide = true;
    private Integer launchFadeInDuration = 0;
    private Integer launchFadeOutDuration = 200;
    private String resourceName = "splash";
    private boolean immersive = false;
    private boolean fullScreen = false;
    private ScaleType scaleType = ScaleType.FIT_XY;
    private boolean usingDialog = false;
    private String layoutName;

    public Integer getBackgroundColor() {
        return backgroundColor;
    }

    public void setBackgroundColor(Integer backgroundColor) {
        this.backgroundColor = backgroundColor;
    }

    public Integer getSpinnerStyle() {
        return spinnerStyle;
    }

    public void setSpinnerStyle(Integer spinnerStyle) {
        this.spinnerStyle = spinnerStyle;
    }

    public Integer getSpinnerColor() {
        return spinnerColor;
    }

    public void setSpinnerColor(Integer spinnerColor) {
        this.spinnerColor = spinnerColor;
    }

    public boolean isShowSpinner() {
        return showSpinner;
    }

    public void setShowSpinner(boolean showSpinner) {
        this.showSpinner = showSpinner;
    }

    public Integer getLaunchShowDuration() {
        return launchShowDuration;
    }

    public void setLaunchShowDuration(Integer launchShowDuration) {
        this.launchShowDuration = launchShowDuration;
    }

    public boolean isLaunchAutoHide() {
        return launchAutoHide;
    }

    public void setLaunchAutoHide(boolean launchAutoHide) {
        this.launchAutoHide = launchAutoHide;
    }

    public Integer getLaunchFadeInDuration() {
        return launchFadeInDuration;
    }

    public String getResourceName() {
        return resourceName;
    }

    public void setResourceName(String resourceName) {
        this.resourceName = resourceName;
    }

    public boolean isImmersive() {
        return immersive;
    }

    public void setImmersive(boolean immersive) {
        this.immersive = immersive;
    }

    public boolean isFullScreen() {
        return fullScreen;
    }

    public void setFullScreen(boolean fullScreen) {
        this.fullScreen = fullScreen;
    }

    public ScaleType getScaleType() {
        return scaleType;
    }

    public void setScaleType(ScaleType scaleType) {
        this.scaleType = scaleType;
    }

    public boolean isUsingDialog() {
        return usingDialog;
    }

    public void setUsingDialog(boolean usingDialog) {
        this.usingDialog = usingDialog;
    }

    public String getLayoutName() {
        return layoutName;
    }

    public void setLayoutName(String layoutName) {
        this.layoutName = layoutName;
    }

    public Integer getLaunchFadeOutDuration() {
        return launchFadeOutDuration;
    }

    public void setLaunchFadeOutDuration(Integer launchFadeOutDuration) {
        this.launchFadeOutDuration = launchFadeOutDuration;
    }
}
