#pragma once

#include "system.h"
#include "debug.h"
#include "shaders.h"
#include "opengl.h"

#include <vector>
#include <unordered_map>
#include <fstream>
using namespace std;
#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
using namespace sf;
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
using namespace glm;

#include "settings.h"
#include "form.h"
#include "transform.h"
#include "camera.h"


class ComponentRendererDeferred : public Component
{
	////////////////////////////////////////////////////////////
	// Component
	////////////////////////////////////////////////////////////

	GLuint shd_forms, shd_light, shd_fxaa, shd_screen;
	GLuint fbo_forms, fbo_light, fbo_fxaa;
	unordered_map<string, GLuint> forms_targets, light_uniforms, light_targets, fxaa_uniforms, fxaa_targets, screen_uniforms;

	struct Texture { GLuint Id; GLenum Type, InternalType, Format; };
	struct Pass { GLuint Framebuffer; GLuint Program; vector<GLuint> Textures; };

	void Init()
	{
		Opengl::InitGlew();

		Pipeline();

		Resize();

		Listeners();
	}

	void Update()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_forms);
		Forms(shd_forms);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_light);
		Quad(shd_light, light_uniforms);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_fxaa);
		Quad(shd_fxaa, fxaa_uniforms);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		Quad(shd_screen, screen_uniforms);

		Opengl::Test();
	}

	void Listeners()
	{
		Event->Listen<Keyboard::Key>("InputKeyReleased", [=](Keyboard::Key Code){
			auto stg = Global->Get<StorageSettings>("settings");
			switch(Code)
			{
			case Keyboard::Key::F2:
				stg->Wireframe = !stg->Wireframe;
				break;
			case Keyboard::Key::F3:
				stg->Verticalsync = !stg->Verticalsync;
				auto wnd = Global->Get<RenderWindow>("window");
				wnd->setVerticalSyncEnabled(stg->Verticalsync);
			}
		});

		Event->Listen("WindowRecreated", [=]{
			glClearColor(.4f,.6f,.9f,0.f);
			Resize();
		});

		Event->Listen<Vector2u>("WindowResize", [=](Vector2u Size){
			Resize(Size);
		});
	}

	void Pipeline()
	{
		shd_forms = Shaders::Create("shaders/forms.vert", "shaders/forms.frag");
		forms_targets.insert(make_pair("position", create_texture("position", GL_RGBA32F).Id));
		forms_targets.insert(make_pair("normal",   create_texture("normal",   GL_RGBA32F).Id));
		forms_targets.insert(make_pair("albedo",   create_texture("albedo",   GL_RGBA32F).Id));
		fbo_forms = Framebuffer(shd_forms, forms_targets, create_texture("depth", GL_DEPTH_COMPONENT32F).Id);

		shd_light = Shaders::Create("shaders/quad.vert", "shaders/light.frag");
		light_uniforms.insert(make_pair("position_tex", get_texture("position").Id));
		light_uniforms.insert(make_pair("normal_tex",   get_texture("normal"  ).Id));
		light_uniforms.insert(make_pair("albedo_tex",   get_texture("albedo"  ).Id));

		light_targets.insert(make_pair("image", create_texture("light", GL_RGBA32F).Id));
		fbo_light = Framebuffer(shd_light, light_targets);

		shd_fxaa = Shaders::Create("shaders/quad.vert", "shaders/fxaa.frag");
		fxaa_uniforms.insert(make_pair("image_tex", get_texture("light").Id));
		fxaa_targets.insert(make_pair("image", create_texture("fxaa", GL_RGBA32F).Id));
		fbo_fxaa = Framebuffer(shd_fxaa, fxaa_targets);

		shd_screen = Shaders::Create("shaders/quad.vert", "shaders/screen.frag");
		screen_uniforms.insert(make_pair("image_tex", get_texture("fxaa").Id));
	}

	void Resize()
	{
		Resize(Global->Get<RenderWindow>("window")->getSize());
	}

	void Resize(Vector2u Size)
	{
		auto stg = Global->Get<StorageSettings>("settings");

		glViewport(0, 0, Size.x, Size.y);

		GLuint program /**/ = shd_forms;
		glUseProgram(program);
		mat4 Projection = perspective(stg->Fieldofview, (float)Size.x / (float)Size.y, 1.0f, stg->Viewdistance);
		glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, value_ptr(Projection));

		for(auto i : Textures)
			resize_texture(i.first, Size);
	}

	////////////////////////////////////////////////////////////
	// Old Pass Functions
	////////////////////////////////////////////////////////////

	GLuint Framebuffer(GLuint shader, unordered_map<string, GLuint> targets)
	{
		GLuint framebuffer;
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

		vector<GLenum> buffers;
		for(auto i : targets)
		{
			int n = buffers.size();
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + n, GL_TEXTURE_2D, i.second, 0);
			buffers.push_back(GL_COLOR_ATTACHMENT0 + n);
		}
		glDrawBuffers(targets.size(), &buffers[0]);

		Debug::PassFail("Renderer framebuffer creation", (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		return framebuffer;
	}

	GLuint Framebuffer(GLuint shader, unordered_map<string, GLuint> targets, GLuint depth)
	{
		GLuint framebuffer;
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

		vector<GLenum> buffers;
		for(auto i : targets)
		{
			int n = buffers.size();
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + n, GL_TEXTURE_2D, i.second, 0);
			buffers.push_back(GL_COLOR_ATTACHMENT0 + n);
		}
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		glDrawBuffers(targets.size(), &buffers[0]);

		Debug::PassFail("Renderer framebuffer creation", (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		return framebuffer;
	}

	////////////////////////////////////////////////////////////
	// New Passes Functions
	////////////////////////////////////////////////////////////

	vector<pair<string, Pass>> Passes;
	Pass get_pass(string name)
	{
		for(auto i : Passes)
			if(i.first == name)
				return i.second;
		throw std::out_of_range("There is no pass with the name (" + name + ").");
	}
	Pass get_pass(uint index)
	{
		if(index < Passes.size())
			return Passes[index].second;
		throw std::out_of_range("There is no pass with the index (" + to_string(index) + ").");
	}

	unordered_map<string, Texture> Textures;
	Texture get_texture(string name)
	{
		auto i = Textures.find(name);
		if(i != Textures.end())
			return i->second;
		throw std::out_of_range("There is no texture with the name (" + name + ").");
	}
	Texture get_or_create_texture(string name, GLenum internal_type)
	{
		auto i = Textures.find(name);
		if(i != Textures.end())
			return i->second;
		return create_texture(name, internal_type);
	}

	Texture create_texture(string name, GLenum internal_type)
	{
		auto i = Textures.find(name);
		if(i != Textures.end())
			throw std::logic_error("There is already a texture with the name (" + name + ").");

		Texture texture;
		glGenTextures(1, &texture.Id);

		texture.InternalType = internal_type;
		switch (internal_type)
		{
		case GL_RGBA16:
		case GL_RGBA16F:
		case GL_RGBA32F:
			texture.Type = GL_RGBA;
			break;
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
		case GL_DEPTH_COMPONENT32F:
			texture.Type = GL_DEPTH_COMPONENT;
		}
		switch (internal_type)
		{
		case GL_RGBA16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
			texture.Format = GL_UNSIGNED_BYTE;
			break;
		case GL_RGBA16F:
		case GL_RGBA32F:
		case GL_DEPTH_COMPONENT32F:
			texture.Format = GL_FLOAT;
			break;
		}

		Textures.insert(make_pair(name, texture));
		resize_texture(name);
		return texture;
	}
	void resize_texture(string name)
	{
		resize_texture(name, Global->Get<RenderWindow>("window")->getSize());
	}
	void resize_texture(string name, Vector2u size)
	{
		Texture texture = get_texture(name);
		glBindTexture(GL_TEXTURE_2D, texture.Id);
		glTexImage2D(GL_TEXTURE_2D, 0, texture.InternalType, size.x, size.y, 0, texture.Type, texture.Format, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	GLuint create_framebuffer(vector<string> targets, bool depth = false)
	{
		GLuint id;
		glGenFramebuffers(1, &id);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

		vector<GLenum> buffers;
		for(auto i : targets)
		{
			int n = buffers.size();
			Texture texture = get_texture(i);
			GLenum attachment;
			switch(texture.Type)
			{
			case GL_RGBA:
				attachment = GL_COLOR_ATTACHMENT0 + n;
				buffers.push_back(attachment);
				break;
			case GL_DEPTH_COMPONENT:
				attachment = GL_DEPTH_ATTACHMENT;
				break;
			}
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.Id, 0);
		}
		glDrawBuffers(buffers.size(), &buffers[0]);

		Debug::PassFail("Renderer framebuffer creation", (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		return id;
	}

	Pass create_pass(string name, string fragmentpath, string target, vector<string> textures)
	{
		vector<string> targets; targets.push_back(target);
		pair<string, string> shaderpaths("shaders/quad.vert", fragmentpath);
		return create_pass(name, shaderpaths, targets, textures);
	}
	Pass create_pass(string name, pair<string, string> shaderpaths, vector<string> targets, vector<string> textures = vector<string>())
	{
		Pass pass;
		pass.Program = Shaders::Create(shaderpaths.first, shaderpaths.second);
		pass.Framebuffer = create_framebuffer(targets);
		for(auto i : textures)
			pass.Textures.push_back(get_texture(i).Id);
		Passes.push_back(make_pair(name, pass));
		return pass;
	}

	////////////////////////////////////////////////////////////
	// Draw Functions
	////////////////////////////////////////////////////////////

	void Quad(GLuint shader, unordered_map<string, GLuint> uniforms)
	{
		glUseProgram(shader);
		glClear(GL_COLOR_BUFFER_BIT);

		int n = 0; for(auto i : uniforms)
		{
			glActiveTexture(GL_TEXTURE0 + n);
			glBindTexture(GL_TEXTURE_2D, i.second);
			glUniform1i(glGetUniformLocation(shader, i.first.c_str()), n);
			n++;
		}

		glBegin(GL_QUADS);
			glVertex2i(0, 0);
			glVertex2i(1, 0);
			glVertex2i(1, 1);
			glVertex2i(0, 1);
		glEnd();

		glActiveTexture(GL_TEXTURE0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);		
		glUseProgram(0);
	}

	void Forms(GLuint shader)
	{
		auto stg = Global->Get<StorageSettings>("settings");
		auto fms = Entity->Get<StorageForm>();
		
		glUseProgram(shader);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(Global->Get<StorageCamera>("camera")->View));

		glPolygonMode(GL_FRONT_AND_BACK, Global->Get<StorageSettings>("settings")->Wireframe ? GL_LINE : GL_FILL);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);

		for(auto i = fms.begin(); i != fms.end(); ++i)
		{
			auto frm = Entity->Get<StorageForm>(i->first);
			auto tsf = Entity->Get<StorageTransform>(i->first);

			mat4 Scale      = scale    (mat4(1), tsf->Scale);
			mat4 Translate  = translate(mat4(1), tsf->Position);
			mat4 Rotate     = rotate   (mat4(1), tsf->Rotation.x, vec3(1, 0 ,0))
							* rotate   (mat4(1), tsf->Rotation.y, vec3(0, 1, 0))
							* rotate   (mat4(1), tsf->Rotation.z, vec3(0, 0, 1));
			mat4 Model = Translate * Rotate * Scale;
			glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(Model));

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, frm->Vertices);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, frm->Normals);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, frm->Texcoords);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frm->Elements);

			glBindTexture(GL_TEXTURE_2D, frm->Texture);

			int count; glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &count);
			glDrawElements(GL_TRIANGLES, count/sizeof(GLuint), GL_UNSIGNED_INT, 0);
		}

		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}
};