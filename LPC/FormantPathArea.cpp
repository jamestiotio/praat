/* FormantPathArea.cpp
 *
 * Copyright (C) 2020-2023 David Weenink, 2022 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FormantPathArea.h"
#include "EditorM.h"

Thing_implement (FormantPathArea, SoundAnalysisArea, 0);

#include "Prefs_define.h"
#include "FormantPathArea_prefs.h"
#include "Prefs_install.h"
#include "FormantPathArea_prefs.h"
#include "Prefs_copyToInstance.h"
#include "FormantPathArea_prefs.h"

/*
	Fast selection of an interval:
	If the mouse click was near a ceiling line in the SoundAnalysisArea we select the whole interval
	at that ceiling frequency.
*/
bool structFormantPathArea :: v_mouse (GuiDrawingArea_MouseEvent event, double x_world, double localY_fraction) {
	const double fmin = our instancePref_spectrogram_viewFrom ();
	const double fmax = our instancePref_spectrogram_viewTo ();
	const double frequencyAtClickPoint = fmin + localY_fraction * (fmax - fmin);
	integer candidate;
	double tmin, tmax;
	FormantPath_getCandidateAtTime (our formantPath(), x_world, & tmin, & tmax, & candidate);
	if (candidate > 0 ) {
		const double ceilingFrequency = our formantPath() -> ceilings [candidate];
		if (fabs ((ceilingFrequency - frequencyAtClickPoint) / (fmax - fmin)) < 0.02) {
			setSelection (tmin, tmax);
			return true;
		}
	}
	return FunctionEditor_defaultMouseInWideDataView (our functionEditor(), event, x_world);
}

static void FormantPathArea_drawCeilings (FormantPathArea me, double tmin, double tmax, double fmin, double fmax) {
	TextGrid path = my formantPath() -> path.get();
	IntervalTier intervalTier = static_cast<IntervalTier> (path -> tiers ->at [1]);
	const integer minIndex = IntervalTier_timeToLowIndex (intervalTier, tmin);
	const integer maxIndex = IntervalTier_timeToHighIndex (intervalTier, tmax);
	Melder_assert (minIndex > 0 && maxIndex > 0);
	Graphics_setWindow (my graphics(), tmin, tmax, fmin, fmax);
	Graphics_setTextAlignment (my graphics(), kGraphics_horizontalAlignment::CENTRE, Graphics_BASELINE);
	Graphics_setColour (my graphics(), Melder_RED);
	Graphics_setLineWidth (my graphics(), 3.0);
	for (integer iint = minIndex; iint <= maxIndex; iint ++) {
		TextInterval textInterval = intervalTier -> intervals.at [iint];
		conststring32 label = textInterval -> text.get();
		if (label) {
			const integer index = Melder_atoi (label);
			if (index > 0 && index <= my formantPath() -> ceilings.size) {
				const double ceiling = my formantPath() -> ceilings [index];
				Graphics_line (my graphics(), textInterval -> xmin, ceiling, textInterval -> xmax, ceiling);
				Graphics_text (my graphics(), 0.5 * (textInterval -> xmin + textInterval -> xmax), ceiling + 50.0, Melder_fixed (ceiling, 0));
			}
		}
	}
	Graphics_setLineWidth (my graphics(), 1.0);
}

void structFormantPathArea :: v_draw_analysis_formants () {
	if (our instancePref_formant_show()) {
		Graphics_setColour (our graphics(), Melder_RED);
		Graphics_setSpeckleSize (our graphics(), our instancePref_formant_dotSize());
		MelderColour oddColour = MelderColour_fromColourName (our instancePref_formant_path_oddColour());
		MelderColour evenColour = MelderColour_fromColourName (our instancePref_formant_path_evenColour());

		Formant_drawSpeckles_inside (our d_formant.get(), our graphics(), our startWindow(), our endWindow(),
			our instancePref_spectrogram_viewFrom(), our instancePref_spectrogram_viewTo(),
			our instancePref_formant_dynamicRange(), oddColour, evenColour, true
		);
		Graphics_setColour (our graphics(), Melder_PINK);
		FormantPathArea_drawCeilings (this, our startWindow(), our endWindow(),
				our instancePref_spectrogram_viewFrom(), our instancePref_spectrogram_viewTo());
		Graphics_setColour (our graphics(), Melder_BLACK);
	}
}


static void menu_cb_FormantSettings (FormantPathArea me, EDITOR_ARGS) {
	EDITOR_FORM (U"Formant analysis settings...", U"Sound: To FormantPath (burg)...")
		REAL (timeStep, U"Time step (s)", my default_formant_path_timeStep ())
		POSITIVE (maximumNumberOfFormants, U"Max. number of formants", my default_formant_path_maximumNumberOfFormants ())
		REAL (middleFormantCeiling, U"Middle formant ceiling (Hz)", my default_formant_path_middleFormantCeiling ())
		POSITIVE (windowLength, U"Window length (s)", my default_formant_path_windowLength ())
		POSITIVE (preEmphasisFrom, U"Pre-emphasis from (Hz)", my default_formant_path_preEmpasisFrom ())
		LABEL (U"The maximum and minimum ceiling frequencies are determined as:")
		LABEL (U" middleFormantCeiling * exp(+/- ceilingStepSize * numberOfStepsUpDown).")
		POSITIVE (ceilingStepSize, U"Ceiling step size", my default_formant_path_ceilingStepSize ())
		NATURAL (numberOfStepsUpDown, U"Number of steps up / down", my default_formant_path_numberOfStepsUpDown ())
	EDITOR_OK
		SET_REAL (timeStep, my instancePref_formant_path_timeStep ())
		SET_REAL (maximumNumberOfFormants, my instancePref_formant_path_maximumNumberOfFormants ())
		SET_REAL (middleFormantCeiling, my instancePref_formant_path_middleFormantCeiling ())
		SET_REAL (windowLength, my instancePref_formant_path_windowLength ())
		SET_REAL (preEmphasisFrom, my instancePref_formant_path_preEmpasisFrom ())
		SET_REAL (ceilingStepSize, my instancePref_formant_path_ceilingStepSize ())
		SET_INTEGER (numberOfStepsUpDown, my instancePref_formant_path_numberOfStepsUpDown ())
	EDITOR_DO
		Melder_require (my sound (),
			U"There is no sound to analyze.");
		my setInstancePref_formant_path_timeStep (timeStep);
		my setInstancePref_formant_path_maximumNumberOfFormants (maximumNumberOfFormants);
		my setInstancePref_formant_path_middleFormantCeiling (middleFormantCeiling);
		my setInstancePref_formant_path_windowLength (windowLength);
		my setInstancePref_formant_path_preEmpasisFrom (preEmphasisFrom);
		my setInstancePref_formant_path_ceilingStepSize (ceilingStepSize);
		my setInstancePref_formant_path_numberOfStepsUpDown (numberOfStepsUpDown);
		autoFormantPath result = Sound_to_FormantPath_burg (my sound (), timeStep, maximumNumberOfFormants, middleFormantCeiling, windowLength, 
			preEmphasisFrom, ceilingStepSize, numberOfStepsUpDown
		);
		my formantPath() -> nx = result -> nx;
		my formantPath() -> dx = result -> dx;
		my formantPath() -> x1 = result -> x1;
		my formantPath() -> formantCandidates = result -> formantCandidates.move();
		my formantPath() -> ceilings = result -> ceilings.move();
		my formantPath() -> path = result -> path.move();
		my d_formant = FormantPath_extractFormant (my  formantPath());
		FunctionArea_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_FormantColourSettings (FormantPathArea me, EDITOR_ARGS) {
	EDITOR_FORM (U"Formant colour settings", nullptr)
		WORD (oddPathColour_string, U"Dots in F1, F3, F5", my default_formant_path_oddColour())
		WORD (evenPathColour_string, U"Dots in F2, F4", my default_formant_path_evenColour())
	EDITOR_OK
		SET_STRING (oddPathColour_string, my instancePref_formant_path_oddColour())
		SET_STRING (evenPathColour_string, my instancePref_formant_path_evenColour())
	EDITOR_DO
		my setInstancePref_formant_path_oddColour (oddPathColour_string);
		my setInstancePref_formant_path_evenColour (evenPathColour_string);
		FunctionArea_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_DrawVisibleFormantContour (FormantPathArea me, EDITOR_ARGS) {
	EDITOR_FORM (U"Draw visible formant contour", nullptr)
		my v_form_pictureWindow (cmd);   // BUG: move to area
		my v_form_pictureMargins (cmd);
		my v_form_pictureSelection (cmd);
		BOOLEAN (garnish, U"Garnish", true)
	EDITOR_OK
		my v_ok_pictureWindow (cmd);   // BUG: move to area
		my v_ok_pictureMargins (cmd);
		my v_ok_pictureSelection (cmd);
		SET_BOOLEAN (garnish, my instancePref_formant_picture_garnish())
	EDITOR_DO
		my v_do_pictureWindow (cmd);   // BUG: move to area
		my v_do_pictureMargins (cmd);
		my v_do_pictureSelection (cmd);
		my setInstancePref_formant_picture_garnish (garnish);
		if (! my instancePref_formant_show())
			Melder_throw (U"No formant contour is visible.\nFirst choose \"Show formant\" from the Formant menu.");
		DataGui_openPraatPicture (me);
		//FormantPath formantPath = (FormantPath) my data;
		//const Formant formant = formantPath -> formant.get();
		//const Formant defaultFormant = formantPath -> formants.at [formantPath -> defaultFormant];
		Formant_drawSpeckles (my d_formant.get(), my pictureGraphics(), my startWindow(), my endWindow(),
			my instancePref_spectrogram_viewTo(), my instancePref_formant_dynamicRange(),
			garnish
		);
		FunctionArea_garnishPicture (me);
		DataGui_closePraatPicture (me);
	EDITOR_END
}

static void menu_cb_showFormants (FormantPathArea me, EDITOR_ARGS) {
	my setInstancePref_formant_show (! my instancePref_formant_show());   // toggle
	GuiMenuItem_check (my formantToggle, my instancePref_formant_show());   // in case we're called from a script
	FunctionEditor_redraw (my functionEditor());
}

static void INFO_DATA__formantListing (FormantPathArea me, EDITOR_ARGS) {
	INFO_DATA
		const double startTime = my startSelection(), endTime = my endSelection();
		MelderInfo_open ();
		MelderInfo_writeLine (U"Time_s   F1_Hz   F2_Hz   F3_Hz   F4_Hz");
		if (startTime == endTime) {
			const double f1 = Formant_getValueAtTime (my d_formant.get(), 1, startTime, kFormant_unit::HERTZ);
			const double f2 = Formant_getValueAtTime (my d_formant.get(), 2, startTime, kFormant_unit::HERTZ);
			const double f3 = Formant_getValueAtTime (my d_formant.get(), 3, startTime, kFormant_unit::HERTZ);
			const double f4 = Formant_getValueAtTime (my d_formant.get(), 4, startTime, kFormant_unit::HERTZ);
			MelderInfo_writeLine (Melder_fixed (startTime, 6), U"   ", Melder_fixed (f1, 6), U"   ", Melder_fixed (f2, 6), U"   ", Melder_fixed (f3, 6), U"   ", Melder_fixed (f4, 6));
		} else {
			integer i1, i2;
			Sampled_getWindowSamples (my d_formant.get(), startTime, endTime, & i1, & i2);
			for (integer i = i1; i <= i2; i ++) {
				const double t = Sampled_indexToX (my d_formant.get(), i);
				const double f1 = Formant_getValueAtTime (my d_formant.get(), 1, t, kFormant_unit::HERTZ);
				const double f2 = Formant_getValueAtTime (my d_formant.get(), 2, t, kFormant_unit::HERTZ);
				const double f3 = Formant_getValueAtTime (my d_formant.get(), 3, t, kFormant_unit::HERTZ);
				const double f4 = Formant_getValueAtTime (my d_formant.get(), 4, t, kFormant_unit::HERTZ);
				MelderInfo_writeLine (Melder_fixed (t, 6), U"   ", Melder_fixed (f1, 6), U"   ", Melder_fixed (f2, 6), U"   ", Melder_fixed (f3, 6), U"   ", Melder_fixed (f4, 6));
			}
		}
		MelderInfo_close ();
	INFO_DATA_END
}
void structFormantPathArea :: v_createMenuItems_formant (EditorMenu menu) {
	our formantToggle = FunctionAreaMenu_addCommand (menu, U"Show formants",
		GuiMenu_CHECKBUTTON | ( our instancePref_formant_show() ? GuiMenu_TOGGLE_ON : 0 ),
		menu_cb_showFormants, this
	);
	// The following menu item should not be visible if there is no sound
	// djmw: 20230211 I don't know how to test whether a sound is available here
	FunctionAreaMenu_addCommand (menu, U"Formant analysis settings...", 0,
			menu_cb_FormantSettings, this);
	FunctionAreaMenu_addCommand (menu, U"Formant colour settings...", 0,
			menu_cb_FormantColourSettings, this);
	FunctionAreaMenu_addCommand (menu, U"Draw visible formant contour...", 0,
			menu_cb_DrawVisibleFormantContour, this);
	FunctionAreaMenu_addCommand (menu, U"Formant listing", 0,
			INFO_DATA__formantListing, this);
}


/* End of file FormantPathArea.cpp */
