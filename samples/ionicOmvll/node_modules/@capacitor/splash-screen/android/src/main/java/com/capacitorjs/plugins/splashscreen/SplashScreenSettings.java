package com.capacitorjs.plugins.splashscreen;

public class SplashScreenSettings {

    private Integer showDuration = 3000;
    private Integer fadeInDuration = 200;
    private Integer fadeOutDuration = 200;
    private boolean autoHide = true;

    public Integer getShowDuration() {
        return showDuration;
    }

    public void setShowDuration(Integer showDuration) {
        this.showDuration = showDuration;
    }

    public Integer getFadeInDuration() {
        return fadeInDuration;
    }

    public void setFadeInDuration(Integer fadeInDuration) {
        this.fadeInDuration = fadeInDuration;
    }

    public Integer getFadeOutDuration() {
        return fadeOutDuration;
    }

    public void setFadeOutDuration(Integer fadeOutDuration) {
        this.fadeOutDuration = fadeOutDuration;
    }

    public boolean isAutoHide() {
        return autoHide;
    }

    public void setAutoHide(boolean autoHide) {
        this.autoHide = autoHide;
    }
}
