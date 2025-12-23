#pragma once

namespace SystemInit {
    void initSerialConnector();
    void factoryResetIfDesired();
    void applicationResetIfDesired();
    void initDisplay();
    void initFont();
    void loadNvsConfig();
}
