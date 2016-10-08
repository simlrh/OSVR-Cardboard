// Internal Includes
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/AnalogInterfaceC.h>
#include "SettingsWindow.h"


// Library/third-party includes


// Standard includes
#include <iostream>

// Anonymous namespace to avoid symbol collision
namespace {

class HardwareDetection {
  public:
    HardwareDetection() : m_found(false) {

	}
    OSVR_ReturnCode operator()(OSVR_PluginRegContext ctx) {
        if (!m_found) {
            m_found = true;

            /// Create our device object
            osvr::pluginkit::registerObjectForDeletion(
                ctx, new OSVRCardboard::SettingsWindow(ctx));
        }
        return OSVR_RETURN_SUCCESS;
    }

  private:
    /// @brief Have we found our device yet? (this limits the plugin to one
    /// instance)
    bool m_found;

};

} // namespace

OSVR_PLUGIN(je_nourish_cardboard) {
    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}
