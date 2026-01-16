package com.kraxarn.chirp3d;

import org.libsdl.app.SDLActivity;

public class Chirp3dActivity extends SDLActivity {
	@Override
	protected String[] getLibraries() {
		return new String[] {
			"SDL3",
			"chirp3d"
		};
	}
}
