#include "weather_icons.h"

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

static void ctopstr(char *s) {
    int len = strlen(s);
    if (len > 255)
        len = 255;
    memmove(s + 1, s, len);
    s[0] = len;
}

#define appleMenuID 1
#define fileMenuID  2

#define mAppleAbout  1
#define mFileRefresh 1
#define mFileQuit    3

WindowPtr mainWindow;
MenuHandle appleMenu, fileMenu;
Rect windowRect = {40, 10, 315, 230};

typedef struct {
    char time[6];
    uint8_t icon;
    char temp;
} HourlyForecast;

typedef struct {
    char date[12];
    char day[4];
    uint8_t icon;
    char hi;
    char lo;
} ForecastDay;

typedef struct {
    uint8_t seq;
    char location[32];
    char temp_current;
    uint8_t condition;
    uint8_t humidity;
    uint8_t wind_speed;
    uint8_t wind_dir;
    HourlyForecast hourly[5];
    ForecastDay forecast[3];
    char updated[16];
} WeatherData;

static WeatherData weather = {
    0,
    "Chicago",
    10,
    2,
    65,
    5,
    2,
    {{"09:00", 0, 8}, {"12:00", 1, 12}, {"15:00", 2, 11}, {"18:00", 0, 9}, {"21:00", 0, 7}},
    {{"3/14", "Mon", 1, 11, 5}, {"3/15", "Tue", 4, 13, 7}, {"3/16", "Wed", 2, 9, 4}},
    "Mar 13 09:43"
};

static char temp_unit[8] = "\xA1"
                           "C";
static char wind_unit[8] = "km/h";

static uint8_t last_seq = 0;
static long last_poll_ticks = 0;
#define POLL_INTERVAL_TICKS (30 * 60)

int fetch_weather_data(void) {
    uint8_t cmd_data[1];
    uint8_t resp_data[256];
    uint8_t len;
    uint8_t status;

    cmd_data[0] = last_seq;
    ipc_send_command(CMD_GET_WEATHER_DATA, 1, cmd_data);

    status = ipc_get_response(&len, resp_data, 255);

    if (status == WEATHER_STATUS_UPDATED && len >= sizeof(WeatherData)) {
        memcpy(&weather, resp_data, sizeof(WeatherData));
        last_seq = weather.seq;
        return 1;
    }

    return 0;
}

void fetch_units(void) {
    uint8_t cmd_data[1] = {0};
    uint8_t resp_data[256];
    uint8_t len;
    uint8_t status;

    ipc_send_command(CMD_GET_UNITS, 0, cmd_data);
    status = ipc_get_response(&len, resp_data, 255);

    if (status == 0 && len >= 16) {
        memcpy(temp_unit, resp_data, 8);
        temp_unit[7] = '\0';
        memcpy(wind_unit, resp_data + 8, 8);
        wind_unit[7] = '\0';
    }
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

void draw_header(void) {
    char loc[32];

    strcpy(loc, weather.location);
    ctopstr(loc);

    MoveTo(10, 18);
    TextSize(12);
    TextFace(bold);
    DrawString((StringPtr)loc);
    TextFace(0);
}

void draw_current_weather(void) {
    char buf[20];
    unsigned char *icon;

    icon = get_icon_large_by_condition(weather.condition);
    draw_icon_large(icon, 31, 38);

    sprintf(buf, "%d%s", weather.temp_current, temp_unit);
    ctopstr(buf);
    TextSize(20);
    TextFace(bold);
    MoveTo(113, 42);
    DrawString((StringPtr)buf);
    TextFace(0);

    TextSize(12);
    MoveTo(116, 60);
    switch (weather.condition) {
    case 0:
        DrawString("\pSunny");
        break;
    case 1:
        DrawString("\pCloudy");
        break;
    case 2:
        DrawString("\pRain");
        break;
    case 3:
        DrawString("\pSnow");
        break;
    case 4:
        DrawString("\pPartly Cloudy");
        break;
    default:
        DrawString("\pUnknown");
        break;
    }

    sprintf(buf, "Humidity: %d%%", weather.humidity);
    ctopstr(buf);
    MoveTo(116, 77);
    DrawString((StringPtr)buf);

    sprintf(buf, "Wind: %d%s", weather.wind_speed, wind_unit);
    ctopstr(buf);
    MoveTo(116, 92);
    DrawString((StringPtr)buf);
}

void draw_hourly(void) {
    char buf[16];
    int i;
    int x = 12;

    for (i = 0; i < 5; i++) {
        char time[6];
        strcpy(time, weather.hourly[i].time);
        ctopstr(time);
        MoveTo(x, 116);
        TextSize(9);
        DrawString((StringPtr)time);

        draw_icon(get_icon_by_condition(weather.hourly[i].icon), x + 4, 122);

        sprintf(buf, "%d%s", weather.hourly[i].temp, temp_unit);
        ctopstr(buf);
        MoveTo(x + 4, 149);
        DrawString((StringPtr)buf);

        x += 42;
    }
    TextSize(12);
}

void draw_forecast(void) {
    char buf[16];
    int y = 175;
    int i;

    MoveTo(14, y);
    DrawString("\pDay");
    MoveTo(164, y);
    DrawString("\pHi/Lo");

    y += 5;
    MoveTo(14, y);
    LineTo(204, y);

    for (i = 0; i < 3; i++) {
        char dateday[16];
        y += 20;

        sprintf(dateday, "%s (%s)", weather.forecast[i].date, weather.forecast[i].day);
        ctopstr(dateday);
        MoveTo(14, y);
        DrawString((StringPtr)dateday);

        draw_icon(get_icon_by_condition(weather.forecast[i].icon), 114, y - 12);

        sprintf(buf, "%d/%d", weather.forecast[i].hi, weather.forecast[i].lo);
        ctopstr(buf);
        MoveTo(164, y);
        DrawString((StringPtr)buf);
    }
}

void draw_footer(void) {
    char buf[32];

    MoveTo(10, 265);
    TextSize(9);
    sprintf(buf, "Updated: %s", weather.updated);
    ctopstr(buf);
    DrawString((StringPtr)buf);
    TextSize(12);
}

void draw_content(void) {
    SetPort(mainWindow);
    EraseRect(&mainWindow->portRect);

    draw_header();
    draw_current_weather();
    draw_hourly();
    draw_forecast();
    draw_footer();
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
        if (itemID == mFileRefresh) {
            if (fetch_weather_data()) {
                InvalRect(&mainWindow->portRect);
            }
        } else if (itemID == mFileQuit) {
            ExitToShell();
        }
        HiliteMenu(0);
        break;
    }
}

void main_loop(void) {
    EventRecord event;
    long current_ticks;
    WindowPtr window;

    while (1) {
        SystemTask();

        current_ticks = TickCount();
        if (current_ticks - last_poll_ticks >= POLL_INTERVAL_TICKS) {
            last_poll_ticks = current_ticks;
            if (fetch_weather_data()) {
                InvalRect(&mainWindow->portRect);
            }
        }

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
                    if (window != FrontWindow()) {
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
        AppendMenu(appleMenu, "\pAbout Weather");
        AppendMenu(appleMenu, "\p-");
        AppendResMenu(appleMenu, 'DRVR');
        InsertMenu(appleMenu, 0);

        fileMenu = NewMenu(fileMenuID, "\pFile");
        AppendMenu(fileMenu, "\pRefresh");
        AppendMenu(fileMenu, "\p-");
        AppendMenu(fileMenu, "\pQuit");
        InsertMenu(fileMenu, 0);
    } else {
        SetMenuBar(mbar);
        AppendMenu(GetMenu(appleMenuID), "\pAbout Weather");
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
    fetch_units();

    mainWindow =
        NewWindow(NULL, &windowRect, "\pWeather", true, documentProc, (WindowPtr)-1, true, 0);
    SetPort(mainWindow);

    draw_content();

    main_loop();
}