#include <Devices.h>
#include <Dialogs.h>
#include <Events.h>
#include <Fonts.h>
#include <Menus.h>
#include <Quickdraw.h>
#include <Strings.h>
#include <TextEdit.h>
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "../common/esp_ipc.h"

typedef struct {
    uint8_t seq;
    uint8_t backlight;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
} HwState;

static HwState hw_state = {0, 100, 0, 0, 0};
static uint8_t last_seq = 0;

static void ctopstr(char *s) {
    int len = strlen(s);
    if (len > 255)
        len = 255;
    memmove(s + 1, s, len);
    s[0] = len;
}

#define appleMenuID 1
#define fileMenuID  2

#define mAppleAbout 1
#define mFileReset  1
#define mFileQuit   3

WindowPtr mainWindow;
MenuHandle appleMenu, fileMenu;
Rect windowRect = {40, 10, 300, 230};

#define CTRL_BACKLIGHT 1
#define CTRL_LED_R     2
#define CTRL_LED_G     3
#define CTRL_LED_B     4
#define CTRL_RESET     5

ControlHandle backlightCtrl;
ControlHandle ledRCtrl;
ControlHandle ledGCtrl;
ControlHandle ledBCtrl;
ControlHandle resetBtn;

int fetch_hw_state(void) {
    uint8_t cmd_data[1];
    uint8_t resp_data[32];
    uint8_t len;
    uint8_t status;

    cmd_data[0] = last_seq;
    ipc_send_command(CMD_GET_HW_STATE, 1, cmd_data);

    status = ipc_get_response(&len, resp_data, 31);

    if (status == HW_STATUS_UPDATED && len >= sizeof(HwState)) {
        memcpy(&hw_state, resp_data, sizeof(HwState));
        last_seq = hw_state.seq;
        return 1;
    }

    return 0;
}

void send_backlight(uint8_t value) {
    uint8_t cmd_data[1];
    uint8_t len;
    cmd_data[0] = value;
    ipc_send_command(CMD_SET_BACKLIGHT, 1, cmd_data);
    ipc_get_response(&len, NULL, 0);
}

void send_led_rgb(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t cmd_data[3];
    uint8_t len;
    cmd_data[0] = r;
    cmd_data[1] = g;
    cmd_data[2] = b;
    ipc_send_command(CMD_SET_LED_RGB, 3, cmd_data);
    ipc_get_response(&len, NULL, 0);
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

void draw_section_header(const char *title, int y) {
    char buf[32];
    strcpy(buf, title);
    ctopstr(buf);
    MoveTo(10, y);
    TextSize(12);
    TextFace(bold);
    DrawString((StringPtr)buf);
    TextFace(0);

    MoveTo(10, y + 3);
    LineTo(210, y + 3);
}

void draw_backlight_status(void) {
    char buf[32];

    sprintf(buf, "Current: %d%%", hw_state.backlight);
    ctopstr(buf);
    MoveTo(10, 38);
    TextSize(12);
    DrawString((StringPtr)buf);
}

void draw_led_status(void) {
    char buf[32];

    sprintf(buf, "Current: R:%d G:%d B:%d", hw_state.led_r, hw_state.led_g, hw_state.led_b);
    ctopstr(buf);
    MoveTo(10, 103);
    TextSize(12);
    DrawString((StringPtr)buf);
}

void draw_rgb_labels(void) {
    MoveTo(10, 123);
    DrawString("\pR:");

    MoveTo(10, 148);
    DrawString("\pG:");

    MoveTo(10, 173);
    DrawString("\pB:");
}

void create_controls(void) {
    Rect ctrlRect;

    ctrlRect.top = 50;
    ctrlRect.bottom = 66;
    ctrlRect.left = 30;
    ctrlRect.right = 200;
    backlightCtrl = NewControl(mainWindow, &ctrlRect, "\p", true, hw_state.backlight, 1, 100,
                               scrollBarProc, CTRL_BACKLIGHT);

    ctrlRect.top = 123;
    ctrlRect.bottom = 139;
    ctrlRect.left = 30;
    ctrlRect.right = 200;
    ledRCtrl = NewControl(mainWindow, &ctrlRect, "\p", true, hw_state.led_r, 0, 255, scrollBarProc,
                          CTRL_LED_R);

    ctrlRect.top = 148;
    ctrlRect.bottom = 164;
    ledGCtrl = NewControl(mainWindow, &ctrlRect, "\p", true, hw_state.led_g, 0, 255, scrollBarProc,
                          CTRL_LED_G);

    ctrlRect.top = 173;
    ctrlRect.bottom = 189;
    ledBCtrl = NewControl(mainWindow, &ctrlRect, "\p", true, hw_state.led_b, 0, 255, scrollBarProc,
                          CTRL_LED_B);

    ctrlRect.top = 221;
    ctrlRect.bottom = 241;
    ctrlRect.left = 150;
    ctrlRect.right = 210;
    resetBtn = NewControl(mainWindow, &ctrlRect, "\pReset", true, 0, 0, 1, pushButProc, CTRL_RESET);
}

void draw_content(void) {
    SetPort(mainWindow);
    EraseRect(&mainWindow->portRect);

    draw_section_header("Backlight", 20);
    draw_backlight_status();

    draw_section_header("RGB LED", 85);
    draw_led_status();
    draw_rgb_labels();

    DrawControls(mainWindow);
}

void update_controls_from_state(void) {
    SetControlValue(backlightCtrl, hw_state.backlight);
    SetControlValue(ledRCtrl, hw_state.led_r);
    SetControlValue(ledGCtrl, hw_state.led_g);
    SetControlValue(ledBCtrl, hw_state.led_b);
}

void handle_scrollbar(ControlHandle ctrl, short part, Point localPt) {
    if (part == inUpButton || part == inDownButton || part == inPageUp || part == inPageDown ||
        part == inThumb) {
        TrackControl(ctrl, localPt, NULL);
        short value = GetControlValue(ctrl);
        if (ctrl == backlightCtrl) {
            hw_state.backlight = value;
            send_backlight(value);
        } else if (ctrl == ledRCtrl) {
            hw_state.led_r = value;
            send_led_rgb(hw_state.led_r, hw_state.led_g, hw_state.led_b);
        } else if (ctrl == ledGCtrl) {
            hw_state.led_g = value;
            send_led_rgb(hw_state.led_r, hw_state.led_g, hw_state.led_b);
        } else if (ctrl == ledBCtrl) {
            hw_state.led_b = value;
            send_led_rgb(hw_state.led_r, hw_state.led_g, hw_state.led_b);
        }
        InvalRect(&mainWindow->portRect);
    }
}

void handle_control_click(Point localPt) {
    ControlHandle ctrl;
    short part = FindControl(localPt, mainWindow, &ctrl);

    if (part == inButton && ctrl == resetBtn) {
        if (TrackControl(ctrl, localPt, NULL)) {
            hw_state.backlight = 100;
            hw_state.led_r = 0;
            hw_state.led_g = 0;
            hw_state.led_b = 0;
            send_backlight(100);
            send_led_rgb(0, 0, 0);
            update_controls_from_state();
            InvalRect(&mainWindow->portRect);
        }
    } else if (part != 0) {
        handle_scrollbar(ctrl, part, localPt);
    }
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
        if (itemID == mFileReset) {
            hw_state.backlight = 100;
            hw_state.led_r = 0;
            hw_state.led_g = 0;
            hw_state.led_b = 0;
            send_backlight(100);
            send_led_rgb(0, 0, 0);
            update_controls_from_state();
            InvalRect(&mainWindow->portRect);
        } else if (itemID == mFileQuit) {
            ExitToShell();
        }
        HiliteMenu(0);
        break;
    }
}

void main_loop(void) {
    EventRecord event;
    WindowPtr window;
    Point localPt;

    while (1) {
        SystemTask();

        if (GetNextEvent(everyEvent, &event)) {
            switch (event.what) {
            case mouseDown:
                switch (FindWindow(event.where, &window)) {
                case inMenuBar:
                    handle_menu(MenuSelect(event.where));
                    break;
                case inSysWindow:
                    SystemClick(&event, window);
                    break;
                case inDrag:
                    DragWindow(window, event.where, &qd.screenBits.bounds);
                    break;
                case inGoAway:
                    if (TrackGoAway(window, event.where)) {
                        if (window == mainWindow) {
                            DisposeWindow(window);
                            ExitToShell();
                        }
                    }
                    break;
                case inContent:
                    if (window == mainWindow) {
                        localPt = event.where;
                        GlobalToLocal(&localPt);
                        handle_control_click(localPt);
                    } else {
                        SelectWindow(window);
                    }
                    break;
                }
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
                window = (WindowPtr)event.message;
                BeginUpdate(window);
                if (window == mainWindow) {
                    draw_content();
                }
                EndUpdate(window);
                break;
            }
        }
    }
}

void setup_menus(void) {
    Handle mbar;

    mbar = GetNewMBar(128);
    if (!mbar) {
        appleMenu = NewMenu(appleMenuID, "\p\x14");
        AppendMenu(appleMenu, "\pAbout CydCtl");
        AppendMenu(appleMenu, "\p-");
        AppendResMenu(appleMenu, 'DRVR');
        InsertMenu(appleMenu, 0);

        fileMenu = NewMenu(fileMenuID, "\pFile");
        AppendMenu(fileMenu, "\pReset");
        AppendMenu(fileMenu, "\p-");
        AppendMenu(fileMenu, "\pQuit");
        InsertMenu(fileMenu, 0);
    } else {
        SetMenuBar(mbar);
        AppendMenu(GetMenu(appleMenuID), "\pAbout CydCtl");
        AppendMenu(GetMenu(appleMenuID), "\p-");
        AppendResMenu(GetMenu(appleMenuID), 'DRVR');
        appleMenu = GetMenuHandle(appleMenuID);
        fileMenu = GetMenuHandle(fileMenuID);
    }

    DrawMenuBar();
}

void main(void) {
    init_toolbox();
    setup_menus();

    mainWindow =
        NewWindow(NULL, &windowRect, "\pCyd Control", true, documentProc, (WindowPtr)-1, true, 0);
    SetPort(mainWindow);

    create_controls();

    if (fetch_hw_state()) {
        update_controls_from_state();
    }

    draw_content();

    main_loop();
}