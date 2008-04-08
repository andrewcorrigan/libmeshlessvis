/*
libMeshlessVis
Copyright (C) 2008 Andrew Corrigan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef WXMAINh
#define WXMAINh

#include <meshless_vis.h>
#include <GL/glew.h>
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/tglbtn.h>
#include <vector>
#include "local_glsl.h"


// Define a new application type
class MyApp: public wxApp
{
public:
	bool OnInit();
	int OnExit();
};

// Define a new frame type
class MeshlessVisCanvas;

class MeshlessVisFrame: public wxFrame
{
public:
	MeshlessVisFrame(wxFrame *frame, const wxString& title, const wxPoint& pos, const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);
	MeshlessVisCanvas* meshless_vis_canvas;

protected:
	void OnClose(wxCloseEvent& event);
	void OnIdle(wxIdleEvent& event);

	DECLARE_EVENT_TABLE()
};

class MeshlessVisCanvas: public wxGLCanvas
{
public:
	MeshlessVisCanvas(wxWindow *parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxString& name = wxT("MeshlessVisCanvas"));

	~MeshlessVisCanvas();

	void create_image(int Nx, int Ny);
	void Cleanup();

	void SetColorMapping(int color_mapping);
	int frames;


protected:
	
	void OnPaint(wxPaintEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void SaveImageToFile(wxString filename);
	
	void show_bounding_box();
	void projection_to_image();
	
	void destroy_image();
	void show_image();
	void project_data_to_image();

	GLSL_DisplayImage* glsl_display_image;
	GLSL_ColorBar* glsl_color_bar;


	DECLARE_EVENT_TABLE()
};

class OptionsFrame: public wxFrame {
public:
    OptionsFrame(int number_of_meshless_datasets, wxWindow* parent, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

	int GetCutoffU();
	int GetCutoffV();
    int2 GetCutoff();

	int GetNumberOfSamplesU();
	int GetNumberOfSamplesV();

	float GetStepSizeU();
	float GetStepSizeV();

	float GetPhaseU();
	float GetPhaseV();
	
	float GetMaximumIntensity();
	float GetMinimumIntensity();

	float GetXRotation();
	float GetYRotation();
	
	int GetNumberOfPartialSums();

	int GetBlockLength();
	
	int GetColorMapping();
	bool IsAnimating();
	bool IsRecording();
	wxString GetFilename();

	int GetCurrentDatasetIndex();
	VisConfig* vis_config;
	
	bool ShowBoundingBox();

protected:
	void AdjustCutoff();
	void OnRecordButton(wxCommandEvent& event);
	void OnCutoffSlider(wxCommandEvent& event);
	void OnStepSizeSlider(wxCommandEvent& event);
	void OnNumberOfSamplesSlider(wxCommandEvent& event);
	void OnPhaseuSlider(wxCommandEvent& event);
	void OnPhasevSlider(wxCommandEvent& event);
	void OnAnimationSlider(wxCommandEvent& event);
	void OnColorMappingSlider(wxCommandEvent& event);
	void OnMaximumIntensitySlider(wxCommandEvent& event);
	void OnMinimumIntensitySlider(wxCommandEvent& event);
	void OnXRotationSlider(wxCommandEvent& event);
	void OnYRotationSlider(wxCommandEvent& event);
	void OnNumberOfPartialSumsSlider(wxCommandEvent& event);
	void OnBlockLengthSlider(wxCommandEvent& event);
	
	void OnAnimationToggleButton(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);
	void OnAnimationTimer(wxTimerEvent& event);
	void OnFPSTimer(wxTimerEvent& event);
	
	void CheckConfig();

	


	wxStaticText* cutoff_label;
	wxSlider* cutoff_slider;
	static const wxWindowID cutoff_ID = 1;

	wxStaticText* step_size_label;
	wxSlider* step_size_slider;
	static const wxWindowID step_size_ID = 2;

	wxStaticText* number_of_samples_label;
	wxSlider* number_of_samples_slider;
	static const wxWindowID number_of_samples_ID = 3;
	std::vector<int> allowed_number_of_samples;

	wxStaticText* phaseu_label;
	wxStaticText* phasev_label;
	wxSlider* phaseu_slider;
	wxSlider* phasev_slider;
	static const wxWindowID phaseu_ID = 4;
	static const wxWindowID phasev_ID = 5;

	wxToggleButton* animation_button;
	wxSlider* animation_slider;
	static const wxWindowID animation_ID = 6;
	static const wxWindowID animation_button_ID = 7;
	wxTimer* animation_timer;
	static const wxWindowID animation_timer_ID = 8;
	wxTimer* fps_timer;
	static const wxWindowID fps_timer_ID = 9;
	
	wxStaticText* fps_label;
	wxStaticText* fps_number_label;
	
	wxStaticText* color_mapping_label;
	wxSlider* color_mapping_slider;
	static const wxWindowID color_mapping_ID = 10;

	wxStaticText* maximum_intensity_label;
	wxSlider* maximum_intensity_slider;
	static const wxWindowID maximum_intensity_ID = 11;
	
	wxStaticText* minimum_intensity_label;
	wxSlider* minimum_intensity_slider;
	static const wxWindowID minimum_intensity_ID = 12;

	wxStaticText* x_rotation_label;
	wxSlider* x_rotation_slider;
	static const wxWindowID x_rotation_ID = 13;
	
	wxStaticText* y_rotation_label;
	wxSlider* y_rotation_slider;
	static const wxWindowID y_rotation_ID = 14;

	wxToggleButton* record_button;
	wxTextCtrl* record_text;
	static const wxWindowID record_button_ID = 15;
	
	wxStaticText* block_length_label;
	wxSlider* block_length_slider;
	static const wxWindowID block_length_ID = 16;

	wxStaticText* number_of_partial_sums_label;
	wxSlider* number_of_partial_sums_slider;
	static const wxWindowID number_of_partial_sums_ID = 17;

	wxStaticText* bounding_box_label;
	wxCheckBox* bounding_box_check_box;
	
	
	DECLARE_EVENT_TABLE()
};

#endif
