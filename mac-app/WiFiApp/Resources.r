#include "Types.r"
#include "Icons.r"
#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"
#include "Finder.r"

resource 'ICN#' (128) {
    {
        $"0000000000000000"
        $"0000000000000000"
        $"000000000003C000"
        $"007FFE0001FFFF80"
        $"07F00FE01F8001F8"
        $"3E00007C7800001E"
        $"300FF00C003FFC00"
        $"00FFFF0001F00F80"
        $"03C003C0070000E0"
        $"00000000000FF000"
        $"001FF800003E7C00"
        $"00381C0000000000"
        $"0000000000018000"
        $"0001800000018000"
        $"0000000000000000"
        $"0000000000000000",
        $"0000000000000000"
        $"000000000003C000"
        $"007FFE0001FFFF80"
        $"07FFFFE01FFFFFF8"
        $"3FFFFFFC7FFFFFFE"
        $"FFFFFFFFFFFFFFFF"
        $"FFFFFFFF79FFFF9E"
        $"33FFFFCC07FFFFE0"
        $"0FFFFFF01FFFFFF8"
        $"0FFFFFF0073FFCE0"
        $"007FFE0000FFFF00"
        $"00FFFF00007FFE00"
        $"003FFC000007E000"
        $"0007E0000007E000"
        $"0003C00000018000"
        $"0000000000000000"
    }
};

resource 'ics#' (128) {
    {
        $"0000000000000FF0"
        $"381C600607E00E70"
        $"1008018007E00420"
        $"0000018000000000",
        $"000000000FF03FFC"
        $"7FFEFFFF7FFE1FF8"
        $"3FFC1FF80FF00FF0"
        $"07E003C001800000"
    }
};

resource 'FREF' (128) {
    'APPL',
    0,
    ""
};

resource 'BNDL' (128) {
    'WIFI',
    0,
    {
        'ICN#', {
            0, 128
        },
        'FREF', {
            0, 128
        }
    }
};

resource 'DITL' (128) {
    {
        { 80, 70, 100, 140 },
        Button { enabled, "OK" };
        { 10, 20, 26, 200 },
        StaticText { disabled, "Cydintosh WiFi App" };
        { 30, 75, 42, 145 },
        StaticText { disabled, "v1.0.0" };
        { 48, 55, 60, 165 },
        StaticText { disabled, "likeablob @2026" };
    }
};

resource 'ALRT' (128) {
    { 100, 20, 210, 220 },
    128,
    {
        OK, visible, sound1,
        OK, visible, sound1,
        OK, visible, sound1,
        OK, visible, sound1,
    },
    noAutoCenter
};

resource 'MENU' (1) {
    1, textMenuProc;
    allEnabled, enabled;
    apple;
    {
    }
};

resource 'MENU' (2) {
    2, textMenuProc;
    allEnabled, enabled;
    "File";
    {
        "Refresh", noIcon, "R", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

resource 'MENU' (3) {
    3, textMenuProc;
    allEnabled, enabled;
    "Edit";
    {
        " Demo Mode", noIcon, noKey, noMark, plain;
    }
};

resource 'MBAR' (128) {
    { 1, 2, 3 };
};

resource 'STR ' (128) {
    "WiFi"
};

resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    256 * 1024,
    128 * 1024
};

/* Owner resource type definition */
type 'WIFI'
{
    byte;
};

/* Owner resource - required for Finder to display icon */
resource 'WIFI' (0, sysHeap) {
    0
};