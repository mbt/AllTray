/*
 * DisplayManager.vala - Display manager for AllTray.
 * Copyright (C) 2009 Michael B. Trausch <mike@trausch.us>
 * License: GNU GPL v3 as published by the Free Software Foundation.
 */
using GLib;
using Gdk;

namespace AllTray {
	public errordomain DisplayManagerError {
		NO_DISPLAY,
		FAILED
	}

	public class DisplayManager : GLib.Object {
		private static DisplayManager _instance;

		private Gdk.Display _display;
		private Gdk.Screen _screen;
		private Gdk.Window _rootWindow;

		public DisplayManager() throws DisplayManagerError {
			if(DisplayManager._instance == null) {
				this._display = Gdk.Display.open(null);

				Debug.Notification.emit(Debug.Subsystem.Display,
										Debug.Level.Information,
										"Opened display via GDK.");

				if(this._display == null) {
					throw new DisplayManagerError.NO_DISPLAY("Unable to open "+
															 "display.");
				}

				this._screen = this._display.get_default_screen();

				DisplayManager._instance = this;
				this._rootWindow = _screen.get_root_window();

				if(_screen.is_composited()) {
					Debug.Notification.emit(Debug.Subsystem.Display,
											Debug.Level.Information,
											"Running on a composited "+
											"screen.");
				}

				_screen.composited_changed += this.on_composite_state_change;

				Debug.Notification.emit(Debug.Subsystem.Display,
										Debug.Level.Information,
										"Initialized DisplayManager.");
			} else {
				// Reached here?  BUG, not error.
				GLib.error("Bug: Only one instance of AllTray.DisplayManager "+
						   "is allowed.");
			}
		}

		public bool is_selection_installed() {
			bool retval = false;
			Gdk.Atom selection_name =
				Gdk.Atom.intern("_ALLTRAY_IS_RUNNING", false);

			Gdk.Window selection_owner =
				Gdk.selection_owner_get(selection_name);

			if(selection_owner != null) {
				retval = true;
			}

			return(retval);
		}

		private void on_composite_state_change() {
			string msg = "Composite status changed (now " +
				_screen.is_composited().to_string() + ").";

			Debug.Notification.emit(Debug.Subsystem.Display,
									Debug.Level.Information, msg);
		}
	}
}
