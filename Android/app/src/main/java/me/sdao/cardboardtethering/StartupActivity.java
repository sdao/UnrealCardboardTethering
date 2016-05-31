package me.sdao.cardboardtethering;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class StartupActivity extends AppCompatActivity {

    private static final String ACTION_USB_PERMISSION =
            "me.sdao.cardboardtethering.USB_PERMISSION";

    private boolean mReturningFromViewer = false;
    private PendingIntent mPermissionIntent;
    private boolean mReceiverRegistered = false;
    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbAccessory accessory = intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if (accessory != null) {
                            launchViewer(accessory);
                        }
                    }
                    else {
                        showInfoDialog("Permissions error",
                                "Can't continue without the USB permission.");
                    }
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_startup);

        Button connectButton = (Button) findViewById(R.id.connect_button);
        connectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                requestUsbPermission();
            }
        });

        if (getIntent() != null) {
            onNewIntent(getIntent());
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (!mReturningFromViewer) {
            if (intent != null) {
                UsbAccessory accessory = intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
                if (accessory != null) {
                    requestUsbPermission();
                }
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mReceiverRegistered) {
            unregisterReceiver(mUsbReceiver);
            mReceiverRegistered = false;
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (resultCode) {
            case MainActivity.STATUS_OK:
                showInfoDialog("Connection ended", "The connection ended successfully.");
                break;
            case MainActivity.STATUS_NOT_FOUND_ERROR:
                showInfoDialog("USB error", "Could not find the USB device");
                break;
            case MainActivity.STATUS_INITIALIZE_DEVICE_ERROR:
                showInfoDialog("USB error", "Could not initialize the USB device");
                break;
            case MainActivity.STATUS_HANDSHAKE_ERROR:
                showInfoDialog("USB error", "USB handshake failure.");
                break;
            case MainActivity.STATUS_WRITE_ERROR:
                showInfoDialog("USB error", "The USB connection failed (IO write error).");
                break;
            case MainActivity.STATUS_READ_ERROR:
                showInfoDialog("USB error", "The USB connection failed (IO read error).");
                break;
        }
        mReturningFromViewer = true;
    }

    private void requestUsbPermission() {
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);

        UsbAccessory[] accessories = manager.getAccessoryList();
        if (accessories == null || accessories.length == 0) {
            showInfoDialog("USB error", "No USB accessories are connected.");
            return;
        }

        mPermissionIntent = PendingIntent.getBroadcast(StartupActivity.this,
                0, new Intent(ACTION_USB_PERMISSION), 0);
        registerReceiver(mUsbReceiver, new IntentFilter(ACTION_USB_PERMISSION));
        mReceiverRegistered = true;

        manager.requestPermission(accessories[0], mPermissionIntent);
    }

    private void launchViewer(UsbAccessory accessory) {
        Intent viewerIntent = new Intent(this, MainActivity.class);
        viewerIntent.putExtra(UsbManager.EXTRA_ACCESSORY, accessory);
        startActivityForResult(viewerIntent, 0);
    }

    private void showInfoDialog(String title, String message) {
        new AlertDialog.Builder(StartupActivity.this)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("Got it", null)
                .create()
                .show();
    }
}
