// Double quotes, spaces OK.
#define PLUG_MFR "AHarker"
#define PLUG_NAME "Octet Violins"

// No quotes or spaces.
#define PLUG_CLASS_NAME OctetViolins

// OSX crap.
// - Manually edit the info.plist file to set the CFBundleIdentifier to the either the string 
// "com.BUNDLE_MFR.audiounit.BUNDLE_NAME" or "com.BUNDLE_MFR.vst.BUNDLE_NAME".
// Double quotes, no spaces.
#define BUNDLE_MFR "AHarker"
#define BUNDLE_NAME "OctetViolins"

// - Manually create a PLUG_CLASS_NAME.exp file with two entries: _PLUG_ENTRY and _PLUG_VIEW_ENTRY
// (these two defines, each with a leading underscore).
// No quotes or spaces.
#define PLUG_ENTRY OctetViolins_Entry
#define PLUG_VIEW_ENTRY OctetViolins_ViewEntry

// The same strings, with double quotes.  There's no other way, trust me.
#define PLUG_ENTRY_STR "OctetViolins_Entry"
#define PLUG_VIEW_ENTRY_STR "OctetViolins_ViewEntry"

// This is the exported cocoa view class, some hosts display this string.
// No quotes or spaces.
#define VIEW_CLASS OctetViolins_View
#define VIEW_CLASS_STR "OctetViolins_View"

// This is interpreted as 0xMAJR.MN.BG
#define PLUG_VER 0x00010000

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'AHOV'
#define PLUG_MFR_ID 'AHar' // make sure this is not the same as BUNDLE_MFR

#define PLUG_CHANNEL_IO "2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0
#define PLUG_DOES_MIDI 0
#define PLUG_DOES_STATE_CHUNKS 1

#if defined(SA_API) && !defined(OS_IOS)
#include "app_wrapper/app_resource.h"
#endif

