Unreal Cardboard Tethering Plugin
=================================

This plugin for UE4 allows you to use an Android device connected via USB
as a head-mounted display for Unreal content running and rendering on your
PC.

Requirements
------------
The plugin targets Unreal Engine 4.12 and Visual Studio 2015 on Windows. If you
have this setup, you can simply drop the plugin into your project's `Plugins`
folder and it should automatically compile when you reload the Unreal Editor.

This plugin will probably work on other UE4 versions with minor tweaks. It
won't work on non-Windows platforms without major tweaks to replace DirectX
specific code.

How to Use
----------
You need to install the Android app in the `Android` folder. It acts as a
receiver for VR content from the Unreal plugin.

Before connecting, you need to install the drivers for your device. You can do
so from the *Window > Install Drivers* menu item. From the dialog, choose the
device whose name is most similar to your device (e.g. for a Nexus 4, you may
see `Google Inc. MTP`). Then the automated driver installer will install the
appropriate drivers. You will see two UAC dialogs; please confirm for both
of the dialogs.

Features
--------
* Transmits head-tracking orientation and interpupillary distance.
* Streams Unreal viewport image using MJPEG compression.

Pretty Pictures!
----------------

Connection dialog:

![Connection dialog in the Unreal Editor](Documentation/connect_window.png)

VR preview on host machine:

![VR preview window with two eye views](Documentation/vr_preview.png)

Android device with Google Cardboard serving as HMD:

![Cardboard UI for head-mounted display](Documentation/vr_cardboard.png)
