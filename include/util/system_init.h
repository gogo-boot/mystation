#pragma once

namespace SystemInit {
    void initSerialConnector();
    void handleButtonLongPressActions();
    bool wasLongPressHandled();
    void initDisplay();
    void initFont();
    void loadNvsConfig();
}
