 /*
Implementation by Torsten Sadowski 2018
 */

#include "GuiNativeEventLinuxX11.h"

#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include <FCConfig.h>
#include <Base/Console.h>
#include <QMainWindow>

#include <QX11Info>
#include <spnav.h>

#if QT_VERSION >= 0x050000
  #include <X11/Xlib.h>
  #include "GuiRawInputEventFilter.h"
  #undef Bool
  #undef CursorShape
  #undef Expose
  #undef KeyPress
  #undef KeyRelease
  #undef FocusIn
  #undef FocusOut
  #undef FontChange
  #undef None
  #undef Status
  #undef Unsorted
  #undef False
  #undef True
  #undef Complex
#endif // #if QT_VERSION >= 0x050000

Gui::GuiNativeEvent::GuiNativeEvent(Gui::GUIApplicationNativeEventAware *app)
: QObject(app)
{
	mainApp = app;
}

Gui::GuiNativeEvent::~GuiNativeEvent()
{
    if (spnav_close())
        Base::Console().Log("Couldn't disconnect from spacenav daemon\n");
    else
        Base::Console().Log("Disconnected from spacenav daemon\n");
}

void Gui::GuiNativeEvent::initSpaceball(QMainWindow *window)
{
    if (spnav_x11_open(QX11Info::display(), window->winId()) == -1) {
        Base::Console().Log("Couldn't connect to spacenav daemon\n");
    } else {
        Base::Console().Log("Connected to spacenav daemon\n");
		mainApp->setSpaceballPresent(true);

    #if QT_VERSION >= 0x050000
        static auto evFilter( [](void *msg, long *result){
                Q_UNUSED(result);
                auto inst(dynamic_cast<Gui::GUIApplicationNativeEventAware *>(QApplication::instance()));
                if (inst) {
                    return inst->nativeEvent->xcbEventFilter(static_cast<const xcb_client_message_event_t *>(msg));
                } else {
                    return false;
                }
            } );
        mainApp->installNativeEventFilter(new Gui::RawInputEventFilter(evFilter));
    #endif // #if QT_VERSION >= 0x050000
    }
}

#if QT_VERSION >= 0x050000

bool Gui::GuiNativeEvent::xcbEventFilter(const xcb_client_message_event_t *xcb_ev)
{
    spnav_event navEvent;

    // Qt4 used XEvents in native event filters, but Qt5 changed to XCB.  The
    // SpaceNavigator API only works with XEvent, so we need to construct a
    // temporary XEvent with just enough information for spnav_x11_event()
    if ((xcb_ev->response_type & 0x7F) == XCB_CLIENT_MESSAGE) {
        XClientMessageEvent xev;

        xev.type = ClientMessage;
        xev.message_type = xcb_ev->type;
        memcpy(xev.data.b, xcb_ev->data.data8, sizeof(xev.data.b));
        xev.serial = 0; // These are just to squash warnings...
        xev.send_event = 0;
        xev.display = 0;
        xev.window = 0;
        xev.format = 0;

        if (!spnav_x11_event(reinterpret_cast<XEvent *>(&xev), &navEvent)) {
            return false;
        }
    } else {
        return false;
    }
    // navEvent is now initialised

    switch (navEvent.type) {
        case SPNAV_EVENT_MOTION:
        {
            motionDataArray[0] = -navEvent.motion.x;
            motionDataArray[1] = -navEvent.motion.z;
            motionDataArray[2] = -navEvent.motion.y;
            motionDataArray[3] = -navEvent.motion.rx;
            motionDataArray[4] = -navEvent.motion.rz;
            motionDataArray[5] = -navEvent.motion.ry;

            mainApp->postMotionEvent(&motionDataArray[0]);
            return true;
        }

        case SPNAV_EVENT_BUTTON:
        {
            auto buttonEvent(new Spaceball::ButtonEvent());
            buttonEvent->setButtonNumber(navEvent.button.bnum);
            if (navEvent.button.press) {
                buttonEvent->setButtonStatus(Spaceball::BUTTON_PRESSED);
            } else {
                buttonEvent->setButtonStatus(Spaceball::BUTTON_RELEASED);
            }
            mainApp->postButtonEvent(navEvent.button.bnum, navEvent.button.press);
            return true;
        }
        default:
            Base::Console().Log("Unknown spaceball event\n");
            return true;
    } // end switch (navEvent.type) {
}

#else  // if QT_VERSION >= 0x050000

bool Gui::GuiNativeEvent::x11EventFilter(XEvent *event)
{
    /*
    First we check if we have a motion flush event:
    - If there are unprocessed motion events we are in a flooding situation.
      In that case we wait with generating a Spaceball event.
    - A motion event counter of 0 indicates that FreeCAD is ready to process
      the event. A Spaceball event, using the saved motion data, is posted.
    */
    static Display* display = QX11Info::display();
    static Atom motion_flush_event = XInternAtom(display, "FCMotionFlushEvent", false);
    static int nMotionEvents = 0;

    if (event->type == ClientMessage)
    {
        Atom message_type = event->xclient.message_type;
        
        if (message_type == motion_flush_event)
        {
            nMotionEvents--;
            if (nMotionEvents == 0)
            {               
            mainApp->postMotionEvent(&motionDataArray);
            }
            
            return true;
        } // XEvent: motion_flush_event
    } // XEvent: ClientMessage

    /*
    From here on we deal with spacenav events only:
    - motion: The event data is saved and a self addressed flush event
              is sent through the window system (XEvent). 
              In the case of an event flooding, the motion data is added up.
    - button: A Spaceball event is posted (QInputEvent).
    */
    spnav_event navEvent;
    if (!spnav_x11_event(event, &navEvent))
        return false;

    if (navEvent.type == SPNAV_EVENT_MOTION)
    {
        /*
        If the motion data of the preceding event has not been processed
        through posting an Spaceball event (flooding situation), 
        the motion data provided by the incoming event is added to the saved data. 
        */
    	int dx, dy, dz, drx, dry, drz;

        if (nMotionEvents == 0)
        {
            dx = 0;
            dy = 0;
            dz = 0;
            drx = 0;
            dry = 0;
            drz = 0;
        }
        else
        {
            dx = motionDataArray[0];
            dy = motionDataArray[1];
            dz = motionDataArray[2];
            drx = motionDataArray[3];
            dry = motionDataArray[4];
            drz = motionDataArray[5];
        }
        
        motionDataArray[0] = -navEvent.motion.x;
        motionDataArray[1] = -navEvent.motion.z;
        motionDataArray[2] = -navEvent.motion.y;
        motionDataArray[3] = -navEvent.motion.rx;
        motionDataArray[4] = -navEvent.motion.rz;
        motionDataArray[5] = -navEvent.motion.ry;
        
        motionDataArray[0] += dx;
        motionDataArray[1] += dy;
        motionDataArray[2] += dz;
        motionDataArray[3] += drx;
        motionDataArray[4] += dry;
        motionDataArray[5] += drz;
        
        /*
        Send a self addressed flush event through the window system. This will
        trigger a Spaceball event if FreeCAD is ready to do so.
        */
        nMotionEvents++;
        XClientMessageEvent flushEvent;
        
        flushEvent.display = display;
        flushEvent.window = event->xclient.window;
        flushEvent.type = ClientMessage;
        flushEvent.format = 8;    
        flushEvent.message_type = motion_flush_event;
        
        XSendEvent (display, flushEvent.window, False, 0, (XEvent*)&flushEvent); // siehe spnavd, False, 0
        
        return true;
    }

    if (navEvent.type == SPNAV_EVENT_BUTTON)
    {
        this->postButtonEvent(navEvent.button.bnum, navEvent.button.press);
        return true;
    }

    Base::Console().Log("Unknown spaceball event\n");
    return true;
}
#endif  // if/else QT_VERSION >= 0x050000
