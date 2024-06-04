package com.capacitorjs.plugins.camera;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.view.View;
import android.view.Window;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import java.util.List;

public class CameraBottomSheetDialogFragment extends BottomSheetDialogFragment {

    interface BottomSheetOnSelectedListener {
        void onSelected(int index);
    }

    interface BottomSheetOnCanceledListener {
        void onCanceled();
    }

    private BottomSheetOnSelectedListener selectedListener;
    private BottomSheetOnCanceledListener canceledListener;
    private List<String> options;
    private String title;

    void setTitle(String title) {
        this.title = title;
    }

    void setOptions(List<String> options, BottomSheetOnSelectedListener selectedListener, BottomSheetOnCanceledListener canceledListener) {
        this.options = options;
        this.selectedListener = selectedListener;
        this.canceledListener = canceledListener;
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        super.onCancel(dialog);
        if (canceledListener != null) {
            this.canceledListener.onCanceled();
        }
    }

    private BottomSheetBehavior.BottomSheetCallback mBottomSheetBehaviorCallback = new BottomSheetBehavior.BottomSheetCallback() {
        @Override
        public void onStateChanged(@NonNull View bottomSheet, int newState) {
            if (newState == BottomSheetBehavior.STATE_HIDDEN) {
                dismiss();
            }
        }

        @Override
        public void onSlide(@NonNull View bottomSheet, float slideOffset) {}
    };

    @Override
    @SuppressLint("RestrictedApi")
    public void setupDialog(Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        if (options == null || options.size() == 0) {
            return;
        }

        Window w = dialog.getWindow();

        final float scale = getResources().getDisplayMetrics().density;

        float layoutPaddingDp16 = 16.0f;
        float layoutPaddingDp12 = 12.0f;
        float layoutPaddingDp8 = 8.0f;
        int layoutPaddingPx16 = (int) (layoutPaddingDp16 * scale + 0.5f);
        int layoutPaddingPx12 = (int) (layoutPaddingDp12 * scale + 0.5f);
        int layoutPaddingPx8 = (int) (layoutPaddingDp8 * scale + 0.5f);

        CoordinatorLayout parentLayout = new CoordinatorLayout(getContext());

        LinearLayout layout = new LinearLayout(getContext());
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(layoutPaddingPx16, layoutPaddingPx16, layoutPaddingPx16, layoutPaddingPx16);
        TextView ttv = new TextView(getContext());
        ttv.setTextColor(Color.parseColor("#757575"));
        ttv.setPadding(layoutPaddingPx8, layoutPaddingPx8, layoutPaddingPx8, layoutPaddingPx8);
        ttv.setText(title);
        layout.addView(ttv);

        for (int i = 0; i < options.size(); i++) {
            final int optionIndex = i;

            TextView tv = new TextView(getContext());
            tv.setTextColor(Color.parseColor("#000000"));
            tv.setPadding(layoutPaddingPx12, layoutPaddingPx12, layoutPaddingPx12, layoutPaddingPx12);
            tv.setText(options.get(i));
            tv.setOnClickListener(
                view -> {
                    if (selectedListener != null) {
                        selectedListener.onSelected(optionIndex);
                    }
                    dismiss();
                }
            );
            layout.addView(tv);
        }

        parentLayout.addView(layout.getRootView());

        dialog.setContentView(parentLayout.getRootView());

        CoordinatorLayout.LayoutParams params = (CoordinatorLayout.LayoutParams) ((View) parentLayout.getParent()).getLayoutParams();
        CoordinatorLayout.Behavior behavior = params.getBehavior();

        if (behavior != null && behavior instanceof BottomSheetBehavior) {
            ((BottomSheetBehavior) behavior).addBottomSheetCallback(mBottomSheetBehaviorCallback);
        }
    }
}
