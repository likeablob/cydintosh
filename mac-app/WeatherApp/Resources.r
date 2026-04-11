#include "Icons.r"
#include "Types.r"
#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"
#include "Finder.r"

resource 'ICN#' (128) {
    {
        $"0000000000000000000000000000000000030100000103000001820000000600"
        $"0000F00000C39C0000E60700000C03380008013000F8018003FC0180030FC180"
        $"0600630006003F000E0001F0180000F8100000C0300000C03000008010000180"
        $"18000F000F801C0001C7380000FDE00000300000000000000000000000000000",
        $"0000000000000000000000000003010000078380000387800003C7000001FF00"
        $"00C3FE0001E7FF0001FFFFB800FFFFFC00FFFFF803FFFFF007FFFFC007FFFFC0"
        $"0FFFFF800FFFFFF01FFFFFF83FFFFFFC3FFFFFF87FFFFFE07FFFFFC03FFFFFC0"
        $"3FFFFF801FFFFF000FFFFC0001FFF80000FDE000003000000000000000000000"
    }
};

resource 'ics#' (128) {
    {
        $"0000000001000090"
        $"00C00D1002041E08"
        $"1050300C00084008"
        $"2030096004000000",
        $"0000000001980190"
        $"09F00FFE0FFE1FF8"
        $"3FF83FFE7FFE7FF8"
        $"7FF83FE00EC00000"
    }
};

/* Finder resources for application icon */

resource 'FREF' (128) {
    'APPL',
    0,
    ""
};

resource 'BNDL' (128) {
    'WTHR',
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
        StaticText { disabled, "Cydintosh Weather App" };
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

resource 'MBAR' (128) {
    { 1, 2 };
};

resource 'STR ' (128) {
    "Weather"
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
type 'WTHR'
{
    byte;
};

/* Owner resource - required for Finder to display icon */
resource 'WTHR' (0, sysHeap) {
    0
};