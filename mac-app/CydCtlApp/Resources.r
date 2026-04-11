#include "Icons.r"
#include "Types.r"
#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"
#include "Finder.r"

/* Owner resource type definition */
type 'CYDC'
{
    byte;
};

resource 'ICN#' (128) {
    {
        $"0000000000000000"
        $"0000000000381C00"
        $"00FE7F0001EFF780"
        $"0383C1C0030000C0"
        $"030000C0030000C0"
        $"0303C0C0070FF0E0"
        $"1E1C38783818181C"
        $"30300C0C30300C0C"
        $"30300C0C30300C0C"
        $"3818181C1E1C3878"
        $"070FF0E00303C0C0"
        $"030000C0030000C0"
        $"030000C00383C1C0"
        $"01EFF78000FE7F00"
        $"00381C0000000000"
        $"0000000000000000",
        $"0000000000000000"
        $"00381C0000FE7F00"
        $"01FFFF8003FFFFC0"
        $"07FFFFE007FFFFE0"
        $"07FFFFE007FFFFE0"
        $"07FFFFE01FFFFFF8"
        $"3FFFFFFC7FFFFFFE"
        $"7FFFFFFE7FFFFFFE"
        $"7FFFFFFE7FFFFFFE"
        $"7FFFFFFE3FFFFFFC"
        $"1FFFFFF807FFFFE0"
        $"07FFFFE007FFFFE0"
        $"07FFFFE007FFFFE0"
        $"03FFFFC001FFFF80"
        $"00FE7F0000381C00"
        $"0000000000000000"
    }
};

resource 'ics#' (128) {
    {
        $"000000000FF01008100813C8666644224422666613C8100810080FF000000000",
        $"00000FF01FF83FFC3FFC7FFEFFFFFFFFFFFFFFFF7FFE3FFC3FFC1FF80FF00000"
    }
};

resource 'FREF' (128) {
    'APPL',
    0,
    ""
};

resource 'BNDL' (128) {
    'CYDC',
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

/* Owner resource - required for Finder to display icon */
resource 'CYDC' (0, sysHeap) {
    0
};

resource 'STR ' (128) {
    "CydCtl"
};

resource 'DITL' (128) {
    {
        { 80, 70, 100, 140 },
        Button { enabled, "OK" };
        { 10, 20, 26, 200 },
        StaticText { disabled, "Cydintosh Control App" };
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
        "Reset", noIcon, "R", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

resource 'MBAR' (128) {
    { 1, 2 };
};

resource 'STR ' (128) {
    "CydCtl"
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