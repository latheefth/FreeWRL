/*
 * $Id$
 *
 * FreeWRL plugin utility functions.
 */

#include "pluginUtils.h"

const char*
XEventToString(int type)
{
    switch(type) {
        case KeyPress:
            return "KeyPress";
        case KeyRelease:
            return "KeyRelease";
        case ButtonPress:
            return "ButtonPress";
        case ButtonRelease:
            return "ButtonRelease";
        case MotionNotify:
            return "MotionNotify";
        case EnterNotify:
            return "EnterNotify";
        case LeaveNotify:
            return "LeaveNotify";
        case FocusIn:
            return "FocusIn";
        case FocusOut:
            return "FocusOut";
        case KeymapNotify:
            return "KeymapNotify";
        case Expose:
            return "Expose";
        case GraphicsExpose:
            return "GraphicsExpose";
        case NoExpose:
            return "NoExpose";
        case VisibilityNotify:
            return "VisibilityNotify";
        case CreateNotify:
            return "CreateNotify";
        case DestroyNotify:
            return "DestroyNotify";
        case UnmapNotify:
            return "UnmapNotify";
        case MapNotify:
            return "MapNotify";
        case MapRequest:
            return "MapRequest";
        case ReparentNotify:
            return "ReparentNotify";
        case ConfigureNotify:
            return "ConfigureNotify";
        case ConfigureRequest:
            return "ConfigureRequest";
        case GravityNotify:
            return "GravityNotify";
        case ResizeRequest:
            return "ResizeRequest";
        case CirculateNotify:
            return "CirculateNotify";
        case CirculateRequest:
            return "CirculateRequest";
        case PropertyNotify:
            return "PropertyNotify";
        case SelectionClear:
            return "SelectionClear";
        case SelectionRequest:
            return "SelectionRequest";
        case SelectionNotify:
            return "SelectionNotify";
        case ColormapNotify:
            return "ColormapNotify";
        case ClientMessage:
            return "ClientMessage";
        case MappingNotify:
            return "MappingNotify";
        case LASTEvent:
            return "LASTEvent";
        default:
            return "Unknown";
    }
}


const char*
XErrorToString(int error)
{
    // from /usr/X11R6/include/X11/X.h
    switch(error) {
        case Success:
            return "Success";
        case BadRequest:
            return "BadRequest";
        case BadValue:
            return "BadValue";
        case BadWindow:
            return "BadWindow";
        case BadPixmap:
            return "BadPixmap";
        case BadAtom:
            return "BadAtom";
        case BadCursor:
            return "BadCursor";
        case BadFont:
            return "BadFont";
        case BadMatch:
            return "BadMatch";
        case BadDrawable:
            return "BadDrawable";
        case BadAccess:
            return "BadAccess";
        case BadAlloc:
            return "BadAlloc";
        case BadColor:
            return "BadColor";
        case BadGC:
            return "BadGC";
        case BadIDChoice:
            return "BadIDChoice";
        case BadName:
            return "BadName";
        case BadLength:
            return "BadLength";
        case BadImplementation:
            return "BadImplementation";
        case FirstExtensionError:
            return "FirstExtensionError";
        case LastExtensionError:
            return "LastExtensionError";
        default:
            return "Unknown";
    }
}
