#include <Devices.h>
#include <Dialogs.h>
#include <Events.h>
#include <Fonts.h>
#include <Menus.h>
#include <Multiverse.h>
#include <Quickdraw.h>
#include <Strings.h>
#include <TextEdit.h>
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "../common/esp_ipc.h"

typedef struct {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
} WifiAP;

typedef struct {
    uint8_t status;
    uint8_t ssid_len;
    char ssid[33];
    uint8_t ip[4];
} WifiStatus;

static uint8_t esp_get_wifi_list(WifiAP *list, uint8_t max_count) {
    ipc_send_command(CMD_GET_WIFI_LIST, 0, NULL);

    uint8_t data[254];
    uint8_t len;
    uint8_t status = ipc_get_response(&len, data, 254);

    if (status != 0 || len == 0) {
        return 0;
    }

    uint8_t count = data[0];
    if (count > max_count)
        count = max_count;

    int pos = 1;
    for (int i = 0; i < count; i++) {
        uint8_t ssid_len = data[pos++];
        if (ssid_len > 32)
            ssid_len = 32;
        memcpy(list[i].ssid, &data[pos], ssid_len);
        list[i].ssid[ssid_len] = '\0';
        pos += ssid_len;
        list[i].rssi = (int8_t)data[pos++];
        list[i].channel = data[pos++];
    }

    return count;
}

static uint8_t esp_get_wifi_status(WifiStatus *status) {
    ipc_send_command(CMD_GET_WIFI_STATUS, 0, NULL);

    uint8_t data[254];
    uint8_t len;
    uint8_t result = ipc_get_response(&len, data, 254);

    if (result != 0 || len < 1) {
        status->status = 0;
        return result;
    }

    status->status = data[0];

    if (len >= 2 && status->status == 2) {
        status->ssid_len = data[1];
        if (status->ssid_len > 32)
            status->ssid_len = 32;
        memcpy(status->ssid, &data[2], status->ssid_len);
        status->ssid[status->ssid_len] = '\0';

        int ip_pos = 2 + status->ssid_len;
        if (len >= ip_pos + 4) {
            memcpy(status->ip, &data[ip_pos], 4);
        }
    }

    return result;
}

static void ctopstr(char *s) {
    int len = strlen(s);
    if (len > 255)
        len = 255;
    memmove(s + 1, s, len);
    s[0] = len;
}

#define appleMenuID 1
#define fileMenuID  2
#define editMenuID  3

#define mAppleAbout   1
#define mFileRefresh  1
#define mFileQuit     3
#define mEditDemoMode 1

WindowPtr mainWindow;
MenuHandle appleMenu, fileMenu, editMenu;
Rect windowRect = {40, 10, 290, 230};

ControlHandle refreshBtn;
ListHandle wifiListView;

WifiAP wifiList[10];
int wifiCount = 0;
WifiStatus wifiStatus;
Boolean ipcAvailable = false;
Boolean demoMode = false;
// Global variable for list Y position (updated in draw_content)
int listTopY = 94;

// Demo data (10 networks) - initialized at runtime
static WifiAP demoWifiList[10];
static WifiStatus demoWifiStatus;

void init_demo_data(void) {
    // Initialize demo APs
    strcpy(demoWifiList[0].ssid, "Home_5G");
    demoWifiList[0].channel = 36;
    demoWifiList[0].rssi = -42;

    strcpy(demoWifiList[1].ssid, "Home_2.4G");
    demoWifiList[1].channel = 1;
    demoWifiList[1].rssi = -55;

    strcpy(demoWifiList[2].ssid, "Office_Guest");
    demoWifiList[2].channel = 11;
    demoWifiList[2].rssi = -63;

    strcpy(demoWifiList[3].ssid, "Starbucks_Free");
    demoWifiList[3].channel = 6;
    demoWifiList[3].rssi = -58;

    strcpy(demoWifiList[4].ssid, "Library_WiFi");
    demoWifiList[4].channel = 1;
    demoWifiList[4].rssi = -71;

    strcpy(demoWifiList[5].ssid, "Neighbor_AP1");
    demoWifiList[5].channel = 11;
    demoWifiList[5].rssi = -68;

    strcpy(demoWifiList[6].ssid, "Cafe_Mocha");
    demoWifiList[6].channel = 6;
    demoWifiList[6].rssi = -52;

    strcpy(demoWifiList[7].ssid, "Train_Station");
    demoWifiList[7].channel = 1;
    demoWifiList[7].rssi = -74;

    strcpy(demoWifiList[8].ssid, "ShoppingMall");
    demoWifiList[8].channel = 36;
    demoWifiList[8].rssi = -49;

    strcpy(demoWifiList[9].ssid, "Park_Public");
    demoWifiList[9].channel = 11;
    demoWifiList[9].rssi = -81;

    // Initialize demo status
    demoWifiStatus.status = 2;
    strcpy(demoWifiStatus.ssid, "Home_5G");
    demoWifiStatus.ssid_len = 9;
    demoWifiStatus.ip[0] = 192;
    demoWifiStatus.ip[1] = 168;
    demoWifiStatus.ip[2] = 1;
    demoWifiStatus.ip[3] = 100;
}

void init_toolbox(void) {
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
}

void draw_content(void) {
    char buf[128];
    int y = 15;

    SetPort(mainWindow);
    EraseRect(&mainWindow->portRect);

    // Draw controls after erasing background
    if (refreshBtn) {
        ShowControl(refreshBtn);
        DrawControls(mainWindow);
    }

    // Draw list view only when data available (real or demo)
    if (wifiListView && (ipcAvailable || demoMode) && wifiCount > 0) {
        LUpdate(mainWindow->visRgn, wifiListView);
    }

    TextFace(bold);
    MoveTo(10, y);
    DrawString("\pWiFi Status");
    TextFace(0);

    y += 3;
    MoveTo(10, y);
    LineTo(210, y);
    y += 12; // 12px gap between separator and content

    if (ipcAvailable || demoMode) {
        if (wifiStatus.status == 2) {
            MoveTo(10, y);
            DrawString("\pConnected:");
            y += 13;

            sprintf(buf, "  %s", wifiStatus.ssid);
            ctopstr(buf);
            MoveTo(10, y);
            DrawString((StringPtr)buf);
            y += 13;

            MoveTo(10, y);
            DrawString("\pIP Address:");
            y += 13;

            sprintf(buf, "  %d.%d.%d.%d", wifiStatus.ip[0], wifiStatus.ip[1], wifiStatus.ip[2],
                    wifiStatus.ip[3]);
            ctopstr(buf);
            MoveTo(10, y);
            DrawString((StringPtr)buf);
        } else {
            MoveTo(10, y);
            DrawString("\pNot Connected");
            y += 13;
            y += 13;
            y += 13;
        }
    } else {
        MoveTo(10, y);
        DrawString("\pNo data available,");
        y += 12;
        MoveTo(10, y);
        DrawString("\pplease Refresh.");
        y += 12;
        y += 12;
    }

    // Available Networks header
    y += 18;
    TextFace(bold);
    MoveTo(10, y);
    DrawString("\pAvailable Networks");
    TextFace(0);

    y += 3;
    MoveTo(10, y);
    LineTo(210, y);

    listTopY = y + 7;
}

void update_wifi_list(void) {
    char buf[64];

    if (!wifiListView)
        return;

    // Clear existing cells
    LDelRow(0, 0, wifiListView);

    // Add networks to list (real or demo)
    if ((ipcAvailable || demoMode) && wifiCount > 0) {
        for (int i = 0; i < wifiCount; i++) {
            LAddRow(1, i, wifiListView);
            Cell cell = {i, 0};
            sprintf(buf, "%-20s ch%2d r%3d", wifiList[i].ssid, wifiList[i].channel,
                    wifiList[i].rssi);
            ctopstr(buf);
            LSetCell(buf + 1, buf[0], cell, wifiListView);
        }
    }
}

void refresh_wifi(void) {
    if (!ipcAvailable) {
        uint8_t ping_result = ipc_ping(0x42);
        if (ping_result == 0) {
            ipcAvailable = true;
        } else {
            InvalRect(&mainWindow->portRect);
            return;
        }
    }

    wifiCount = esp_get_wifi_list(wifiList, 10);
    esp_get_wifi_status(&wifiStatus);
    update_wifi_list();
    InvalRect(&mainWindow->portRect);
}

void handle_null_event(void) {
    // TODO: Auto-refresh disabled to prevent UI freeze during WiFi scan
}

void handle_menu(long menuChoice) {
    int menuID = HiWord(menuChoice);
    int itemID = LoWord(menuChoice);
    Str255 daName;

    switch (menuID) {
    case appleMenuID:
        if (itemID == mAppleAbout) {
            Alert(128, NULL);
        } else if (itemID > 1) {
            GetMenuItemText(appleMenu, itemID, daName);
            OpenDeskAcc(daName);
        }
        HiliteMenu(0);
        break;

    case fileMenuID:
        if (itemID == mFileRefresh && !demoMode) {
            refresh_wifi();
        } else if (itemID == mFileQuit) {
            ExitToShell();
        }
        HiliteMenu(0);
        break;

    case editMenuID:
        if (itemID == mEditDemoMode) {
            demoMode = !demoMode;
            CheckItem(editMenu, mEditDemoMode, demoMode);

            if (demoMode) {
                // Load demo data
                wifiCount = 10;
                for (int i = 0; i < 10; i++) {
                    wifiList[i] = demoWifiList[i];
                }
                wifiStatus = demoWifiStatus;
            } else {
                // Reset to real data if available
                wifiCount = 0;
                wifiStatus.status = 0;
                if (ipcAvailable) {
                    refresh_wifi();
                }
            }
            update_wifi_list();
            InvalRect(&mainWindow->portRect);
        }
        HiliteMenu(0);
        break;
    }
}

void handle_update(EventRecord *event) {
    WindowPtr window = (WindowPtr)event->message;
    if (window == mainWindow) {
        BeginUpdate(window);
        draw_content();
        EndUpdate(window);
    }
}

void handle_mouse_down(EventRecord *event) {
    WindowPtr window;
    long menuChoice;
    Rect dragRect = {0, 0, qd.screenBits.bounds.right, qd.screenBits.bounds.bottom};

    switch (FindWindow(event->where, &window)) {
    case inMenuBar:
        menuChoice = MenuSelect(event->where);
        if (menuChoice != 0) {
            handle_menu(menuChoice);
        }
        break;

    case inSysWindow:
        SystemClick(event, window);
        break;

    case inDrag:
        DragWindow(window, event->where, &dragRect);
        break;

    case inGoAway:
        if (TrackGoAway(window, event->where)) {
            DisposeWindow(window);
            ExitToShell();
        }
        break;

    case inContent:
        if (window != FrontWindow()) {
            SelectWindow(window);
        } else {
            // Handle button click
            ControlHandle ctrl;
            short part;
            Point localPoint = event->where;
            GlobalToLocal(&localPoint);
            part = FindControl(localPoint, window, &ctrl);
            if (part == inButton && ctrl == refreshBtn) {
                if (TrackControl(ctrl, event->where, NULL)) {
                    refresh_wifi();
                }
            }

            // Handle list click
            if (wifiListView) {
                LClick(localPoint, event->modifiers, wifiListView);
            }
        }
        break;
    }
}

void main_loop(void) {
    EventRecord event;

    while (1) {
        SystemTask();

        if (GetNextEvent(everyEvent, &event)) {
            switch (event.what) {
            case mouseDown:
                handle_mouse_down(&event);
                break;

            case keyDown:
            case autoKey:
                if (event.modifiers & cmdKey) {
                    if ((event.message & charCodeMask) == 'q') {
                        ExitToShell();
                    } else {
                        handle_menu(MenuKey(event.message & charCodeMask));
                    }
                }
                break;

            case updateEvt:
                handle_update(&event);
                break;
            }
        } else {
            handle_null_event();
        }
    }
}

void setup_menus(void) {
    Handle mbar;

    mbar = GetNewMBar(128);
    if (!mbar) {
        appleMenu = NewMenu(appleMenuID, "\p\x14");
        AppendMenu(appleMenu, "\pAbout WiFi");
        AppendMenu(appleMenu, "\p-");
        AppendResMenu(appleMenu, 'DRVR');
        InsertMenu(appleMenu, 0);

        fileMenu = NewMenu(fileMenuID, "\pFile");
        AppendMenu(fileMenu, "\pRefresh");
        AppendMenu(fileMenu, "\p-");
        AppendMenu(fileMenu, "\pQuit");
        InsertMenu(fileMenu, 0);

        editMenu = NewMenu(editMenuID, "\pEdit");
        AppendMenu(editMenu, "\pDemo Mode");
        InsertMenu(editMenu, 0);
    } else {
        SetMenuBar(mbar);
        AppendMenu(GetMenu(appleMenuID), "\pAbout WiFi");
        AppendMenu(GetMenu(appleMenuID), "\p-");
        AppendResMenu(GetMenu(appleMenuID), 'DRVR');
        appleMenu = GetMenuHandle(appleMenuID);
        fileMenu = GetMenuHandle(fileMenuID);
        editMenu = GetMenuHandle(editMenuID);
    }

    DrawMenuBar();
}

void main(void) {
    init_demo_data();
    init_toolbox();
    setup_menus();

    mainWindow =
        NewWindow(NULL, &windowRect, "\pWiFi Status", true, documentProc, (WindowPtr)-1, true, 0);
    SetPort(mainWindow);

    wifiStatus.status = 0;
    wifiCount = 0;

    // Create Refresh button at bottom-right of window
    Rect btnRect;
    SetRect(&btnRect, 150, 220, 210, 240);
    refreshBtn = NewControl(mainWindow, &btnRect, "\pRefresh", true, 0, 0, 0, pushButProc, 0);

    // Create WiFi list view with scroll bar
    int listHeight = 210 - listTopY;
    Rect listRect = {listTopY, 7, 210, 198};
    Rect dataBounds = {0, 0, 0, 1};
    Point cellSize = {15, 178};
    wifiListView = LNew(&listRect, &dataBounds, cellSize, 0, mainWindow, true, false, false, true);
    LAddColumn(1, 0, wifiListView);

    uint8_t ping_result = ipc_ping(0x42);
    if (ping_result == 0) {
        ipcAvailable = true;
        refresh_wifi();
    }

    main_loop();
}
