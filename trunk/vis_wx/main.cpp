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

#include "main.h"
#include <string>
#include <cmath>
#include <boost/math/quaternion.hpp>

#include <Magick++.h> 
#include <iostream>

const int CUTOFF_STEPS_INCREMENT = 16;

OptionsFrame* global_options_frame = 0;
MeshlessVisFrame* global_meshless_vis_frame = 0;

 //----------------------------------------------------------------------------
//utility opengl view setup functions
//----------------------------------------------------------------------------
void SetViewport(GLsizei w, GLsizei h) { glViewport(0, 0, w, h); }
void Set2DView(GLsizei w, GLsizei h, GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	SetViewport(w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(l, r, b, t, n, f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
void Set2DView(GLsizei w, GLsizei h, GLfloat l, GLfloat r, GLfloat b, GLfloat t) { Set2DView(w,h,l,r,b,t,0.0f,1.0f); }
void Set2DView(GLsizei w, GLsizei h, GLfloat space_w, GLfloat space_h) { Set2DView(w,h,0,space_w,0,space_h); }
void Set3DView(GLsizei w, GLsizei h)
{	
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)w/(GLfloat)h,0.1f,100.0f);
	glMatrixMode(GL_MODELVIEW);
}

//----------------------------------------------------------------------------
//Variables/Function declarations
//----------------------------------------------------------------------------
struct orthonormal_basis
{
	void set(float th, float ph) { theta = th, phi = ph, compute_basis(); }
	void change(float d_th, float d_ph) { theta += d_th, phi += d_ph, compute_basis(); }
	void compute_basis()
	{
		using namespace boost::math;
		
		quaternion<float> r = quaternion<float>(std::cos(theta/2), 0, std::sin(theta/2), 0)*quaternion<float>(std::cos(phi/2), std::sin(phi/2), 0, 0);
		quaternion<float> r_conj = conj(r);
		
		quaternion<float> rx = r*quaternion<float>(0, 1, 0, 0)*r_conj;
		quaternion<float> ry = r*quaternion<float>(0, 0, 1, 0)*r_conj;
		quaternion<float> rz = r*quaternion<float>(0, 0, 0, 1)*r_conj;
		
		w_u.x = rx.R_component_2(), w_u.y = rx.R_component_3(), w_u.z = rx.R_component_4();
		w_v.x = ry.R_component_2(), w_v.y = ry.R_component_3(), w_v.z = ry.R_component_4();
		w_t.x = rz.R_component_2(), w_t.y = rz.R_component_3(), w_t.z = rz.R_component_4();
	}
	
	float3 w_u;
	float3 w_v;
	float3 w_t;
	float theta, phi;
} basis;

#include "meshless.h"
MeshlessDataset* meshless_datasets = 0;
int number_of_meshless_datasets = 0;

#include "local_fbo.h"
FBO_Projection* fbo_projection = 0;

#include "gpu/texture.h"
GLuint tex_image = 0;

#include "gpu/pbo.h"
GLuint pbo_image = 0;

struct BoundingBox
{
	float min_x, min_y, min_z;
	float max_x, max_y, max_z;
} bounding_box;
void compute_bounding_box();

//-----------------------------------------------------------------------------------------------------------------
int MyApp::OnExit()
{
	delete_meshless_datasets(meshless_datasets, number_of_meshless_datasets);
	return wxApp::OnExit();
}
bool MyApp::OnInit()
{
	try 
	{
		wxString filename;
		if(argc >= 2) {
			filename = argv[1];
		}
		else {
			filename = wxFileSelector(wxT("Choose Meshless Data File"), wxT(""), wxT(""), wxT(""), wxT("SPH Data (*.sph)|*.sph|All files (*.*)|*.*"), wxOPEN);
		}
		
		if(filename.IsEmpty()) return false;

		load_meshless_datasets_from_file(filename.fn_str(), &meshless_datasets, &number_of_meshless_datasets);
		if(meshless_datasets == 0 || number_of_meshless_datasets < 1) {
			wxString msg(wxT("Meshless data file ")); msg << filename << wxT(" is invalid");
			throw msg;
		}
		
		compute_bounding_box();

		MeshlessVisFrame *meshless_vis_frame = new MeshlessVisFrame(0, wxT("Volume Visualization of Meshless Data"), wxDefaultPosition, wxSize(512, 512));
		global_meshless_vis_frame = meshless_vis_frame;

		//textures, pbos and fbos are dependent on parameters set in options and will be set when the options_frame is initialized
		OptionsFrame* options_frame = new OptionsFrame(number_of_meshless_datasets, 0, wxT("Volume Visualization of Meshless Data"));
		global_options_frame = options_frame;
	}
	catch(wxString str)
	{
		wxMessageBox(str, wxT("Error"));
		return false;
	}
	catch(...)
	{
		wxMessageBox(wxT("Unknown exception"), wxT("Error"));
		return false;
	}
	return true;
}
IMPLEMENT_APP(MyApp)

//-----------------------------------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MeshlessVisFrame, wxFrame)
	EVT_CLOSE(MeshlessVisFrame::OnClose)
	EVT_IDLE(MeshlessVisFrame::OnIdle)
END_EVENT_TABLE()

// MeshlessVisFrame constructor
MeshlessVisFrame::MeshlessVisFrame(wxFrame *frame, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(frame, wxID_ANY, title, pos, size, style)
{
	Show();
	meshless_vis_canvas = new MeshlessVisCanvas(this, wxID_ANY, pos, GetClientSize(), wxSUNKEN_BORDER);
}

void MeshlessVisFrame::OnClose(wxCloseEvent& event)
{
	global_meshless_vis_frame = 0;
	meshless_vis_canvas->Cleanup();

	wxFrame::OnCloseWindow(event);
}

void MeshlessVisFrame::OnIdle(wxIdleEvent& event)
{
	if(global_options_frame && global_options_frame->IsAnimating()) this->Refresh();
}

// ---------------------------------------------------------------------------
// MeshlessVisCanvas
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MeshlessVisCanvas, wxGLCanvas)
	EVT_PAINT(MeshlessVisCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(MeshlessVisCanvas::OnEraseBackground)	// prevents flickering
	
END_EVENT_TABLE()



MeshlessVisCanvas::MeshlessVisCanvas(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
: wxGLCanvas(parent, id, pos, size, style|wxFULL_REPAINT_ON_RESIZE, name)
{
	frames = 0;

	// get glew runnning
	if (!GetContext()) throw wxString(wxT("Can't get OpenGL context"));
	SetCurrent();
	if(GLEW_OK != glewInit()) throw wxString(wxT("glewInit failure"));
	if(!GLEW_VERSION_2_1) throw wxString(wxT("OpenGL 2.1 support not detected"));
	if(!GLEW_ARB_texture_float) throw wxString(wxT("ARB_texture_float support not detected"));

	//create shaders and display lists
	glsl_display_image = 0; SetColorMapping(1);
	
	//the image's texture, fbo, and pbo will be initialized once OptionsFrame is
}

void MeshlessVisCanvas::SetColorMapping(int color_mapping)
{
	if(glsl_display_image) {
		if(color_mapping == glsl_display_image->color_mapping) return;
		
		delete glsl_display_image;
	}
	
	glsl_display_image = new GLSL_DisplayImage(color_mapping);
}

void MeshlessVisCanvas::Cleanup()
{
	if (GetContext())
	{
		SetCurrent();
		delete glsl_display_image;
		destroy_image();
	}
}

MeshlessVisCanvas::~MeshlessVisCanvas(){}

void MeshlessVisCanvas::SaveImageToFile(wxString filename)
{
	float* rgb_image = new float[GetClientSize().x*GetClientSize().y*3];
	glReadPixels(0, 0, GetClientSize().x, GetClientSize().y, GL_RGB, GL_FLOAT, rgb_image);
	Magick::Image image;
	image.read(GetClientSize().x, GetClientSize().y, "RGB", Magick::FloatPixel, rgb_image);
	image.flip();
	image.write(std::string(filename.fn_str()));

	delete[] rgb_image;
	
}

void MeshlessVisCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	++frames;
	wxPaintDC dc(this);
	if (!GetContext()) return;
	SetCurrent();


	glClear(GL_COLOR_BUFFER_BIT);
	if(global_options_frame != 0)
	{
		project_data_to_image(); show_image();
		if(global_options_frame->ShowBoundingBox()) show_bounding_box();
	}
	SwapBuffers();
	
	if(global_options_frame != 0 && global_options_frame->IsRecording())
	{
		SaveImageToFile(global_options_frame->GetFilename());
	}
	
}

void MeshlessVisCanvas::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {}

void compute_bounding_box()
{
	bounding_box.max_x = bounding_box.min_x = meshless_datasets[0].groups[0].h_constraints[0].position.x;
	bounding_box.max_y = bounding_box.min_y = meshless_datasets[0].groups[0].h_constraints[0].position.y;
	bounding_box.max_z = bounding_box.min_z = meshless_datasets[0].groups[0].h_constraints[0].position.z;
	
	std::cout << bounding_box.min_x*basis.w_u.x + bounding_box.min_y*basis.w_u.y + bounding_box.min_y*basis.w_u.z <<  ", " << bounding_box.min_x*basis.w_v.x + bounding_box.min_y*basis.w_v.y + bounding_box.min_y*basis.w_v.z << std::endl;
	std::cout << bounding_box.max_x*basis.w_u.x + bounding_box.max_y*basis.w_u.y + bounding_box.max_y*basis.w_u.z <<  ", " << bounding_box.max_x*basis.w_v.x + bounding_box.max_y*basis.w_v.y + bounding_box.max_y*basis.w_v.z << std::endl;

	
	for(int i = 0; i != number_of_meshless_datasets; i++) {

		int number_of_terms = 0;
		for(int j = 0; j != meshless_datasets[i].number_of_groups; j++)
		{
			for(int k = 0; k != meshless_datasets[i].groups[j].number_of_terms; k++)
			{
				float x = meshless_datasets[i].groups[j].h_constraints[k].position.x;
				float y = meshless_datasets[i].groups[j].h_constraints[k].position.y;
				float z = meshless_datasets[i].groups[j].h_constraints[k].position.z;

				if(bounding_box.max_x < x) bounding_box.max_x = x;
				else if (bounding_box.min_x > x) bounding_box.min_x = x;

				if(bounding_box.max_y < y) bounding_box.max_y = y;
				else if (bounding_box.min_y > y) bounding_box.min_y = y;
				
				if(bounding_box.max_z < z) bounding_box.max_z = z;
				else if (bounding_box.min_z > z) bounding_box.min_z = z;
			}
		}
	}
}

float* to_uv(float px, float py, float pz, float pu, float pv)
{
	static float out[2];
	out[0] = (pu)*(1.0f-global_options_frame->GetPhaseU()) + px*basis.w_v.x + py*basis.w_v.y + pz*basis.w_v.z;
	out[1] = (pv)*(1.0f-global_options_frame->GetPhaseV()) + px*basis.w_u.x + py*basis.w_u.y + pz*basis.w_u.z;
	return out;
}

void MeshlessVisCanvas::show_bounding_box()
{
	if(global_options_frame == 0 || number_of_meshless_datasets <= 0 || meshless_datasets == 0) return;

	glColor3f(1,1,1);
	float period_u = 1.0f/global_options_frame->GetStepSizeU(), period_v = 1.0f/global_options_frame->GetStepSizeV();
	Set2DView(GetClientSize().x, GetClientSize().y, 0, period_u, 0, period_v);
	
		float min_x = bounding_box.min_x, min_y = bounding_box.min_y, min_z = bounding_box.min_z;
		float max_x = bounding_box.max_x, max_y = bounding_box.max_y, max_z = bounding_box.max_z;
		glLineWidth(3.0);
		glBegin(GL_LINES);
			glVertex2fv(to_uv(min_x,min_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,min_y,min_z,period_u,period_v));	
	
			glVertex2fv(to_uv(min_x,min_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(min_x,max_y,min_z,period_u,period_v));
	
			glVertex2fv(to_uv(min_x,min_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(min_x,min_y,max_z,period_u,period_v));
			
			glVertex2fv(to_uv(max_x,max_y,max_z,period_u,period_v));
			glVertex2fv(to_uv(min_x,max_y,max_z,period_u,period_v));
	
			glVertex2fv(to_uv(max_x,max_y,max_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,min_y,max_z,period_u,period_v));
	
			glVertex2fv(to_uv(max_x,max_y,max_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,max_y,min_z,period_u,period_v));
	
			glVertex2fv(to_uv(max_x,min_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,min_y,max_z,period_u,period_v));	

			glVertex2fv(to_uv(max_x,min_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,max_y,min_z,period_u,period_v));	

			glVertex2fv(to_uv(min_x,max_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,max_y,min_z,period_u,period_v));	

			glVertex2fv(to_uv(min_x,max_y,min_z,period_u,period_v));
			glVertex2fv(to_uv(min_x,max_y,max_z,period_u,period_v));	
	
			glVertex2fv(to_uv(min_x,min_y,max_z,period_u,period_v));
			glVertex2fv(to_uv(min_x,max_y,max_z,period_u,period_v));	
	
			glVertex2fv(to_uv(min_x,min_y,max_z,period_u,period_v));
			glVertex2fv(to_uv(max_x,min_y,max_z,period_u,period_v));	
		glEnd();
		glLineWidth(1.0);

}

void MeshlessVisCanvas::show_image()
{
	if(global_options_frame == 0 || number_of_meshless_datasets <= 0 || meshless_datasets == 0) return;

	Set2DView(GetClientSize().x, GetClientSize().y, 1.0f, 1.0f);
	float phase_u = global_options_frame->GetPhaseU();
	float phase_v = global_options_frame->GetPhaseV();

	// visualize image by texture mapping the projection
	glsl_display_image->use();
	glUniform1f(glsl_display_image->intensity_max, global_options_frame->GetMaximumIntensity());
	glUniform1f(glsl_display_image->intensity_min, global_options_frame->GetMinimumIntensity());
	glsl_display_image->uniform_tex_2d(0, tex_image, glsl_display_image->image);
	glBegin(GL_QUADS);
		glTexCoord2f(phase_u, phase_v);				glVertex2f(0.0f, 0.0f);
		glTexCoord2f(phase_u+1.0f, phase_v);		glVertex2f(1.0f, 0.0f);
		glTexCoord2f(phase_u+1.0f, phase_v+1.0f);	glVertex2f(1.0f, 1.0f);
		glTexCoord2f(phase_u, phase_v+1.0f);		glVertex2f(0.0f, 1.0f);
	glEnd();
	glUseProgram(0);
}



void MeshlessVisCanvas::destroy_image()
{
	delete_texture(tex_image);
	delete_pbo(pbo_image);
	delete fbo_projection; fbo_projection = 0;		//delete'ng NULL pointer should have no effect
}

void MeshlessVisCanvas::create_image(int Nx, int Ny)
{
	destroy_image();

	try {
		create_texture_2D(tex_image, GL_TEXTURE_2D, GL_LINEAR, GL_REPEAT, GL_LUMINANCE32F_ARB, GL_LUMINANCE, Nx, Ny, 0);
		create_pbo(pbo_image, GL_PIXEL_UNPACK_BUFFER, GL_DYNAMIC_COPY, Nx*Ny*sizeof(float2));
		fbo_projection = new FBO_Projection(Nx, Ny);
	}
	catch(wxString str) {
		wxMessageBox(str, wxT("OpenGL Texture/PBO/FBO Error"));
	}
	catch(...)
	{
		wxMessageBox(wxT("Unknown exception"), wxT("OpenGL Texture/PBO/FBO Error"));
	}
}

void MeshlessVisCanvas::project_data_to_image()
{
	if(global_options_frame == 0 || number_of_meshless_datasets <= 0 || meshless_datasets == 0) return;

	//compute FVR, storing the result in a pixel buffer
	MeshlessDataset* meshless_dataset = &meshless_datasets[global_options_frame->GetCurrentDatasetIndex()];
	vis_register_meshless_dataset(global_options_frame->vis_config, meshless_dataset);
	vis_opengl_fourier_volume_rendering(meshless_dataset, global_options_frame->vis_config, pbo_image);
	vis_unregister_meshless_dataset(global_options_frame->vis_config, meshless_dataset);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_image);
	glBindTexture(GL_TEXTURE_2D, tex_image);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, global_options_frame->vis_config->_number_of_samples.x, global_options_frame->vis_config->_number_of_samples.y, GL_LUMINANCE, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


//------------------------------------------------------------------------------------------------------------------------------------
// OptionsFrame
//------------------------------------------------------------------------------------------------------------------------------------
OptionsFrame::OptionsFrame(int number_of_meshless_datasets, wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
: wxFrame(parent, wxID_ANY, title, pos, size, style)
{
	vis_config = 0;
	
	SetTitle(title);
	SetBackgroundColour(wxNullColour);

	wxGridSizer* grid_sizer = new wxGridSizer(0, 2, 10, 10);
	SetSizer(grid_sizer);

	// create widgets
	cutoff_label = new wxStaticText(this, wxID_ANY, wxT(""));
	cutoff_slider = new wxSlider(this, cutoff_ID, 1, 1, 1, wxDefaultPosition, wxSize(200,30));
	
	step_size_label = new wxStaticText(this, wxID_ANY, wxT(""));
	step_size_slider = new wxSlider(this, step_size_ID, 20, 1, 400, wxDefaultPosition, wxSize(200,30));
		
	allowed_number_of_samples.push_back(64); while(allowed_number_of_samples.back()*2 <= GL_MAX_TEXTURE_SIZE) allowed_number_of_samples.push_back(allowed_number_of_samples.back()*2);
	int N = static_cast<int>(allowed_number_of_samples.size());
	number_of_samples_label = new wxStaticText(this, wxID_ANY, wxT(""));
	number_of_samples_slider = new wxSlider(this, number_of_samples_ID, 0, 0, N-1, wxDefaultPosition, wxSize(200,30));
	
	phaseu_label = new wxStaticText(this, wxID_ANY, wxT(""));
	phaseu_slider = new wxSlider(this, phaseu_ID, 50, 0, 99, wxDefaultPosition, wxSize(200,30));
	phasev_label = new wxStaticText(this, wxID_ANY, wxT(""));
	phasev_slider = new wxSlider(this, phasev_ID, 50, 0, 99, wxDefaultPosition, wxSize(200,30));

	minimum_intensity_label = new wxStaticText(this, wxID_ANY, wxT(""));
	minimum_intensity_slider = new wxSlider(this, minimum_intensity_ID, 0, 0, 99, wxDefaultPosition, wxSize(200,30));
	maximum_intensity_label = new wxStaticText(this, wxID_ANY, wxT(""));
	maximum_intensity_slider = new wxSlider(this, maximum_intensity_ID, 10, 1, 100, wxDefaultPosition, wxSize(200,30));

	x_rotation_label = new wxStaticText(this, wxID_ANY, wxT(""));
	x_rotation_slider = new wxSlider(this, x_rotation_ID, 0, 0, 100, wxDefaultPosition, wxSize(200,30));
	y_rotation_label = new wxStaticText(this, wxID_ANY, wxT(""));
	y_rotation_slider = new wxSlider(this, y_rotation_ID, 0, 0, 100, wxDefaultPosition, wxSize(200,30));

	block_length_label = new wxStaticText(this, wxID_ANY, wxT(""));
	block_length_slider = new wxSlider(this, block_length_ID, 2, 0, 2, wxDefaultPosition, wxSize(200,30));

	number_of_partial_sums_label = new wxStaticText(this, wxID_ANY, wxT(""));
	number_of_partial_sums_slider = new wxSlider(this, number_of_partial_sums_ID, 1, 1, 32, wxDefaultPosition, wxSize(200,30));

	animation_button = new wxToggleButton(this, animation_button_ID, wxT(""));
	animation_slider = new wxSlider(this, animation_ID, 0, 0, number_of_meshless_datasets-1, wxDefaultPosition, wxSize(200,30));

	vis_config = 
		vis_config_create(false,	// we're going to map an opengl texture into _d_image, so we don't let the config automatically allocate memory there
		make_float2(GetStepSizeU(), GetStepSizeV()),
		GetCutoff(),
		basis.w_u, basis.w_v,
		make_int2(GetNumberOfSamplesU(), GetNumberOfSamplesV()),
		GetBlockLength(),
		GetNumberOfPartialSums());
	
	CheckConfig();

	animation_timer = new wxTimer(this, animation_timer_ID);
	animation_timer->Start(50);
	
	fps_timer = new wxTimer(this, fps_timer_ID);
	fps_timer->Start(2500);

	fps_label = new wxStaticText(this,wxID_ANY, wxT("Frames Per Second:"));
	fps_number_label = new wxStaticText(this, wxID_ANY, wxT(""));

	color_mapping_label = new wxStaticText(this, wxID_ANY, wxT(""));
	color_mapping_slider = new wxSlider(this, color_mapping_ID, 1, 1, 3, wxDefaultPosition, wxSize(200,30));

	record_button = new wxToggleButton(this, record_button_ID, wxT(""));
	record_text = new wxTextCtrl(this, wxID_ANY, wxT("output.png"));

	bounding_box_label = new wxStaticText(this, wxID_ANY, wxT("Show Bounding Box"));
	bounding_box_check_box = new wxCheckBox(this, wxID_ANY, wxT(""));
	
	wxCommandEvent wx_command_event;
	OnRecordButton(wx_command_event);
	//OnCutoffSlider is called by OnNumberOfSamplesSlider
	OnStepSizeSlider(wx_command_event);
	OnNumberOfSamplesSlider(wx_command_event);
	OnPhaseuSlider(wx_command_event);
	OnPhasevSlider(wx_command_event);
	OnAnimationSlider(wx_command_event);
	OnAnimationToggleButton(wx_command_event);
	OnColorMappingSlider(wx_command_event);
	OnMaximumIntensitySlider(wx_command_event);
	OnMinimumIntensitySlider(wx_command_event);
	OnXRotationSlider(wx_command_event);
	OnYRotationSlider(wx_command_event);
	OnBlockLengthSlider(wx_command_event);
	OnNumberOfPartialSumsSlider(wx_command_event);
		
	// create the grid sizer and add widgets to it
	grid_sizer->Add(cutoff_label);
	grid_sizer->Add(cutoff_slider);
	grid_sizer->Add(step_size_label);
	grid_sizer->Add(step_size_slider);
	grid_sizer->Add(number_of_samples_label);
	grid_sizer->Add(number_of_samples_slider);
	grid_sizer->Add(phaseu_label);
	grid_sizer->Add(phaseu_slider);
	grid_sizer->Add(phasev_label);
	grid_sizer->Add(phasev_slider);
	grid_sizer->Add(animation_button);
	grid_sizer->Add(animation_slider);
	grid_sizer->Add(fps_label);
	grid_sizer->Add(fps_number_label);
	grid_sizer->Add(color_mapping_label);
	grid_sizer->Add(color_mapping_slider);
	grid_sizer->Add(minimum_intensity_label);
	grid_sizer->Add(minimum_intensity_slider);
	grid_sizer->Add(maximum_intensity_label);
	grid_sizer->Add(maximum_intensity_slider);
	grid_sizer->Add(x_rotation_label);
	grid_sizer->Add(x_rotation_slider);
	grid_sizer->Add(y_rotation_label);
	grid_sizer->Add(y_rotation_slider);
	grid_sizer->Add(record_button);
	grid_sizer->Add(record_text);
	grid_sizer->Add(block_length_label);
	grid_sizer->Add(block_length_slider);
	grid_sizer->Add(number_of_partial_sums_label);
	grid_sizer->Add(number_of_partial_sums_slider);
	grid_sizer->Add(bounding_box_label);
	grid_sizer->Add(bounding_box_check_box);

	grid_sizer->Fit(this);

	Layout();
	Show();
}

void OptionsFrame::OnAnimationTimer(wxTimerEvent& event)
{
	if(animation_button->GetValue()) return;

	if(animation_slider->GetValue() == animation_slider->GetMax()) animation_slider->SetValue(animation_slider->GetMin());
	else animation_slider->SetValue(animation_slider->GetValue()+1);
}

void OptionsFrame::OnFPSTimer(wxTimerEvent& event)
{
	if(global_meshless_vis_frame == 0) return;
	
	wxString label;
	label << global_meshless_vis_frame->meshless_vis_canvas->frames / 2.5;
	fps_number_label->SetLabel(label);

	global_meshless_vis_frame->meshless_vis_canvas->frames = 0;
}

void OptionsFrame::OnClose(wxCloseEvent& event)
{
	global_options_frame = 0;
	vis_config_destroy(vis_config);
	delete animation_timer;
	delete fps_timer;
	wxFrame::OnCloseWindow(event);
}


void OptionsFrame::CheckConfig()
{
	if(!vis_config_check(vis_config))
	{
		wxMessageBox(wxT("Invalid or suboptimal configuration"), wxT("Warning"));
	}
}

int OptionsFrame::GetCutoffU()
{
	return std::min(GetNumberOfSamplesU()/2, std::max(1,cutoff_slider->GetValue()) * CUTOFF_STEPS_INCREMENT);
}

int OptionsFrame::GetCutoffV()
{
	return std::min(GetNumberOfSamplesV()/2, std::max(1,cutoff_slider->GetValue()) * CUTOFF_STEPS_INCREMENT);
}

int2 OptionsFrame::GetCutoff()
{
	return make_int2(GetCutoffU(), GetCutoffV());
}

bool OptionsFrame::ShowBoundingBox()
{
	return bounding_box_check_box->GetValue();
}


int OptionsFrame::GetNumberOfSamplesU() { return allowed_number_of_samples[number_of_samples_slider->GetValue()]; }
int OptionsFrame::GetNumberOfSamplesV() { return GetNumberOfSamplesU(); }

float OptionsFrame::GetStepSizeU() { return step_size_slider->GetValue()*0.005f; }
float OptionsFrame::GetStepSizeV() { return GetStepSizeU(); }

float OptionsFrame::GetPhaseU() { return static_cast<float>(phaseu_slider->GetValue())/100.0f; }
float OptionsFrame::GetPhaseV() { return static_cast<float>(phasev_slider->GetValue())/100.0f; }

int OptionsFrame::GetCurrentDatasetIndex() { return animation_slider->GetValue(); }

int OptionsFrame::GetColorMapping() { return color_mapping_slider->GetValue(); }

float OptionsFrame::GetMaximumIntensity() { return 1e-2f*maximum_intensity_slider->GetValue();}
float OptionsFrame::GetMinimumIntensity() { return 1e-2f*minimum_intensity_slider->GetValue();}

const float PI = 3.14159265358979f;

float OptionsFrame::GetXRotation()
{
	return  2*PI*x_rotation_slider->GetValue() / (x_rotation_slider->GetMax() - x_rotation_slider->GetMin());
}
float OptionsFrame::GetYRotation()
{
	return  2*PI*y_rotation_slider->GetValue() / (y_rotation_slider->GetMax() - y_rotation_slider->GetMin());	
}

int OptionsFrame::GetNumberOfPartialSums() { return number_of_partial_sums_slider->GetValue(); }

int OptionsFrame::GetBlockLength()
{
	switch(block_length_slider->GetValue())
	{
	case 0: return 64;
	case 1: return 128;
	case 2: return 256;

		// if you want to add larger block lengths, you need to increase the increment from 16 to 32 (which don't work anyway)
		
	default: return 64;
	}
}

wxString OptionsFrame::GetFilename() 
{
	wxString filename(wxT("_"));
	int k = GetCurrentDatasetIndex();
	if(k < 10) filename << wxT("0");
	if(k < 100) filename << wxT("0");
	if(k < 1000) filename << wxT("0");
	if(k < 10000) filename << wxT("0");
	if(k < 100000) filename << wxT("0");
	filename << k << wxT("_") << record_text->GetValue();
	return filename;
}

bool OptionsFrame::IsRecording() { return record_button->GetValue(); }

bool OptionsFrame::IsAnimating() { return !animation_button->GetValue(); }


void OptionsFrame::OnRecordButton(wxCommandEvent& WXUNUSED(event))
{
	if(record_button->GetValue()) record_button->SetLabel(wxT("Stop Recording"));
	else record_button->SetLabel(wxT("Start Recording"));
}


void OptionsFrame::OnCutoffSlider( wxCommandEvent& WXUNUSED(event) )
{
	int2 cutoff_frequency = GetCutoff();
	wxString label(wxT("Cutoff ")); label << cutoff_frequency.x;
	cutoff_label->SetLabel(label);

	vis_config_change_cutoff_frequency(vis_config, cutoff_frequency);
	CheckConfig();
}

void OptionsFrame::OnStepSizeSlider( wxCommandEvent& WXUNUSED(event) )
{
	wxString label(wxT("Step size ")); label << GetStepSizeU();
	step_size_label->SetLabel(label);

	vis_config->step_size = make_float2(GetStepSizeU(), GetStepSizeV());
	CheckConfig();
}

void OptionsFrame::AdjustCutoff()
{
	cutoff_slider->SetMin(1);
	cutoff_slider->SetMax( (GetNumberOfSamplesU() / 2) / CUTOFF_STEPS_INCREMENT);
	
	wxCommandEvent wx_command_event;
	OnCutoffSlider(wx_command_event);	//this is NOT automatically called by cutoff_slider->SetMax
}

void OptionsFrame::OnNumberOfSamplesSlider( wxCommandEvent& WXUNUSED(event) )
{
	if(global_meshless_vis_frame == 0 || global_meshless_vis_frame->meshless_vis_canvas == 0) return; //no point in changing any of this if the vis frame is closed

	int2 number_of_samples = make_int2(GetNumberOfSamplesU(), GetNumberOfSamplesV());
	vis_config_change_number_of_samples(vis_config, number_of_samples);	
	
	wxString label(wxT("Number of samples ")); label << number_of_samples.x;
	number_of_samples_label->SetLabel(label);

	AdjustCutoff();
	CheckConfig();

	global_meshless_vis_frame->meshless_vis_canvas->create_image(GetNumberOfSamplesU(), GetNumberOfSamplesV());
}

void OptionsFrame::OnPhaseuSlider(wxCommandEvent& WXUNUSED(event))
{
	wxString label(wxT("U-Phase ")); label << GetPhaseU();
	phaseu_label->SetLabel(label);
}

void OptionsFrame::OnPhasevSlider(wxCommandEvent& WXUNUSED(event))
{
	wxString label(wxT("V-Phase ")); label << GetPhaseV();
	phasev_label->SetLabel(label);
}

void OptionsFrame::OnMaximumIntensitySlider(wxCommandEvent& WXUNUSED(event))
{
	minimum_intensity_slider->SetRange(minimum_intensity_slider->GetMin(), maximum_intensity_slider->GetValue()-1);
	wxString label(wxT("Maximum Intensity ")); label << GetMaximumIntensity();
	maximum_intensity_label->SetLabel(label);
}

void OptionsFrame::OnMinimumIntensitySlider(wxCommandEvent& WXUNUSED(event))
{
	maximum_intensity_slider->SetRange(minimum_intensity_slider->GetValue()+1, maximum_intensity_slider->GetMax());
	wxString label(wxT("Minimum Intensity ")); label << GetMinimumIntensity();
	minimum_intensity_label->SetLabel(label);
}

void OptionsFrame::OnXRotationSlider(wxCommandEvent& WXUNUSED(event))
{
	wxString label(wxT("X-Rotation ")); label << GetXRotation();
	x_rotation_label->SetLabel(label);

	basis.set(GetYRotation(), GetXRotation());
	if(vis_config) vis_config->u_axis = basis.w_u, vis_config->v_axis = basis.w_v;
	CheckConfig();
	if(animation_button->GetValue() && global_meshless_vis_frame != 0 && global_meshless_vis_frame->meshless_vis_canvas != 0) global_meshless_vis_frame->Refresh();
}

void OptionsFrame::OnYRotationSlider(wxCommandEvent& WXUNUSED(event))
{
	wxString label(wxT("Y-Rotation ")); label << GetYRotation();
	y_rotation_label->SetLabel(label);
	
	basis.set(GetYRotation(), GetXRotation());
	if(vis_config) vis_config->u_axis = basis.w_u, vis_config->v_axis = basis.w_v; 
	CheckConfig();
	if(animation_button->GetValue() && global_meshless_vis_frame != 0 && global_meshless_vis_frame->meshless_vis_canvas != 0) global_meshless_vis_frame->Refresh();
}

void OptionsFrame::OnBlockLengthSlider(wxCommandEvent& WXUNUSED(event))
{
	int block_length = GetBlockLength();
	wxString label(wxT("Block length ")); label << block_length;
	block_length_label->SetLabel(label);

	vis_config->block_length = block_length;
	CheckConfig();
	AdjustCutoff();
}

void OptionsFrame::OnNumberOfPartialSumsSlider(wxCommandEvent& WXUNUSED(event))
{
	int number_of_partial_sums = GetNumberOfPartialSums();
	wxString label(wxT("Partial Sums ")); label << number_of_partial_sums;
	number_of_partial_sums_label->SetLabel(label);

	vis_config_change_number_of_partial_sums(vis_config, number_of_partial_sums);
}


void OptionsFrame::OnAnimationSlider(wxCommandEvent& event)
{
	if(animation_button->GetValue() && global_meshless_vis_frame != 0 && global_meshless_vis_frame->meshless_vis_canvas != 0) global_meshless_vis_frame->Refresh();
}

void OptionsFrame::OnAnimationToggleButton(wxCommandEvent& WXUNUSED(event))
{
	if(animation_button->GetValue()) animation_button->SetLabel(wxT("Play"));
	else animation_button->SetLabel(wxT("Pause"));
}

void OptionsFrame::OnColorMappingSlider(wxCommandEvent& event)
{
	int color_mapping = GetColorMapping();
	wxString label;
	switch(color_mapping_slider->GetValue())
	{
		case 1: label << wxT("Red"); break;
		case 2: label << wxT("Rainbow"); break;
		case 3: label << wxT("Blue"); break;
	}
	label << wxT(" Color Mapping");
	color_mapping_label->SetLabel(label);

	if(global_meshless_vis_frame != 0 && global_meshless_vis_frame->meshless_vis_canvas != 0) 
	{
		global_meshless_vis_frame->meshless_vis_canvas->SetColorMapping(color_mapping);
	}
}

BEGIN_EVENT_TABLE(OptionsFrame, wxFrame)
	EVT_CLOSE(OptionsFrame::OnClose)

	EVT_SLIDER(cutoff_ID, OptionsFrame::OnCutoffSlider)
	EVT_SLIDER(step_size_ID, OptionsFrame::OnStepSizeSlider)
	EVT_SLIDER(number_of_samples_ID, OptionsFrame::OnNumberOfSamplesSlider)
	EVT_SLIDER(phaseu_ID, OptionsFrame::OnPhaseuSlider)
	EVT_SLIDER(phasev_ID, OptionsFrame::OnPhasevSlider)
	EVT_SLIDER(minimum_intensity_ID, OptionsFrame::OnMinimumIntensitySlider)
	EVT_SLIDER(maximum_intensity_ID, OptionsFrame::OnMaximumIntensitySlider)
	EVT_SLIDER(x_rotation_ID, OptionsFrame::OnXRotationSlider)
	EVT_SLIDER(y_rotation_ID, OptionsFrame::OnYRotationSlider)
	EVT_SLIDER(block_length_ID, OptionsFrame::OnBlockLengthSlider)
	EVT_SLIDER(number_of_partial_sums_ID, OptionsFrame::OnNumberOfPartialSumsSlider)

	EVT_SLIDER(animation_ID, OptionsFrame::OnAnimationSlider)
	EVT_SLIDER(color_mapping_ID, OptionsFrame::OnColorMappingSlider)

	EVT_TOGGLEBUTTON(animation_button_ID, OptionsFrame::OnAnimationToggleButton)
	EVT_TOGGLEBUTTON(record_button_ID, OptionsFrame::OnRecordButton)

	EVT_TIMER(animation_timer_ID, OptionsFrame::OnAnimationTimer)
	EVT_TIMER(fps_timer_ID, OptionsFrame::OnFPSTimer)
END_EVENT_TABLE()
