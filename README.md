# OSVR-Cardboard

An OSVR plugin to receive orientation data from a compatible mobile app (like [OSVR-Cardboard-iOS](https://github.com/simlrh/OSVR-Cardboard-iOS)), as well as translating the QR code found on Google Cardboard viewers into an OSVR display config file.

##Usage

Add the plugin file to your OSVR plugins folder. Edit osvr_server_config to set the renderManagerConfig to extended mode and to run on your main display by setting xPosition and yPosition to 0.

Start the OSVR server. A window will pop up showing the plugin's status. Use the mobile app to connect to OSVR on the PC. 

You should see a message that a client has connected, and the configuration section should show the name of your Cardboard viewer and phone. If it says "Scan QR Code", go into the settings in the mobile app and configure the viewer by scanning its QR code.

Once the viewer is detected correctly, click Save to save the display config somewhere useful, like the displays folder in the OSVR main directory, and edit the display section of your osvr_server_config.json to point to the new file.  (You only have to do this step when changing Cardboard viewer, phone or phone resolution). Connect again from the mobile app. 

Finally, click on "Viewer resolution" to set your main display resolution to match your phone (or the resolution of the stream to your phone). Now you can launch any OSVR compatible content.

For SteamVR content, you may run into an IPD issue (different combinations of phone and viewer can mean very large center of projection offsets). [This version of the SteamVR driver may fix the issue](https://github.com/simlrh/SteamVR-OSVR/releases/tag/v0.1-dk1).