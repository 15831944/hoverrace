// Controller.h
//
// This file contains the definition for the HoverRace::Client::Controller
// class.  It contains all the code to control input devices; joysticks, 
// keyboards, and the like.

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>

#include "OIS/OIS.h"
#include "OIS/OISInputManager.h"
#include "OIS/OISException.h"
#include "OIS/OISKeyboard.h"
#include "OIS/OISMouse.h"
#include "OIS/OISJoyStick.h"
#include "OIS/OISEvents.h"
#include "../../../engine/Util/Config.h"
#include "../../../engine/Util/OS.h"

#define	CTL_MOTOR_ON	1
#define CTL_LEFT		2
#define CTL_RIGHT		3
#define CTL_JUMP		4
#define CTL_BRAKE		5
#define CTL_FIRE		6
#define CTL_WEAPON		7
#define CTL_LOOKBACK	8

#define AXIS_X			1
#define AXIS_Y			2
#define AXIS_Z			3

#define SET_CONTROL		(WM_USER + 1)

// maybe later... TODO
//#include "OIS/OISForceFeedback.h"

namespace HoverRace {
namespace Client {
namespace Control {

class InputHandler;
typedef boost::shared_ptr<InputHandler> InputHandlerPtr;
class UiHandler;
typedef boost::shared_ptr<UiHandler> UiHandlerPtr;

using namespace OIS;

/***
 * Contains information on the current control state.  Eventually, its members should
 * be made analog instead of digital (well, the ones that can, at least).
 */
struct ControlState {
	// TODO: make these inputs analog, not digital
	bool motorOn;
	bool jump;
	bool brake;
	bool fire;
	bool weapon;
	bool lookBack;
	bool right;
	bool left;
};

/// Convenient typedef prevents us from having to write long namespace specifiers
typedef HoverRace::Util::Config::cfg_control_t InputControl;

/***
 * The HoverRace::Client::Control::Controller class handles all the input of HoverRace.
 * It tracks all of the input devices, loads key mapping configuration, and returns the
 * current control state when it is asked for.  It is meant to replace Richard's old
 * system that was pretty ugly.
 */
class Controller : public KeyListener, public MouseListener, public JoyStickListener {
	public:
		Controller(Util::OS::wnd_t mainWindow, UiHandlerPtr uiHandler);
		~Controller();

		void poll();
		
		ControlState getControlState(int player);
		void captureNextInput(int control, int player, Util::OS::wnd_t hwnd);
		void stopCapture();
		void disableInput(int control, int player);
		bool controlsUpdated();
		void saveControls();
		std::string toString(HoverRace::Util::Config::cfg_control_t control);

		void EnterControlLayer(InputHandlerPtr handler);
		void LeaveControlLayer();
		void ResetControlLayers();

	private:
		void InitInputManager(Util::OS::wnd_t mainWindow);
		void LoadControllerConfig();

		/// OIS input manager does most of the work for us
		InputManager *mgr;

		// now the input devices
		Keyboard *kbd;
		Mouse    *mouse;

		int numJoys;
		JoyStick **joys; /// we can have over 9000 joysticks, depending on how much RAM we have
		int *joyIds;

		// event handler
		bool keyPressed(const KeyEvent &arg);
		bool keyReleased(const KeyEvent &arg);
		bool mouseMoved(const MouseEvent &arg);
		bool mousePressed(const MouseEvent &arg, MouseButtonID id);
		bool mouseReleased(const MouseEvent &arg, MouseButtonID id);
		bool buttonPressed(const JoyStickEvent &arg, int button);
		bool buttonReleased(const JoyStickEvent &arg, int button);
		bool axisMoved(const JoyStickEvent &arg, int axis);
		bool povMoved(const JoyStickEvent &arg, int pov);

		// for polling
		void clearControlState(); // clear the control state before each poll
		bool getSingleControlState(InputControl input);
		void updateAxisControl(bool &ctlState, InputControl &ctl, int *axes, int numAxes);

		void setControls(HoverRace::Util::Config::cfg_controls_t *controls);

		ControlState curState[HoverRace::Util::Config::MAX_PLAYERS];

		// kept locally to make things a bit quicker
		InputControl brake[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl fire[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl jump[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl left[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl lookBack[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl motorOn[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl right[HoverRace::Util::Config::MAX_PLAYERS];
		InputControl weapon[HoverRace::Util::Config::MAX_PLAYERS];

		void getCaptureControl(int captureControl, InputControl **input, Util::Config::cfg_control_t **cfg_input);

		bool captureNext;
		int captureControl;
		int capturePlayerId;
		Util::OS::wnd_t captureHwnd;
		bool updated;

		// mouse inputs
		int mouseXLast;
		int mouseYLast;
		int mouseZLast;

		UiHandlerPtr uiHandler;

		// Control layers.
		typedef std::vector<InputHandlerPtr> controlLayers_t;
		controlLayers_t controlLayers;
};

} // namespace Control
} // namespace Client
} // namespace HoverRace

#endif
