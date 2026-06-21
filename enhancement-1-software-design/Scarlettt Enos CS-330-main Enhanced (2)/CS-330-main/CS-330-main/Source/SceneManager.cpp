///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";

	// Enhancement: I added a helper method for creating materials in one place.
	// This reduces repetition and makes it easier to maintain and create new materials in the future.
	OBJECT_MATERIAL CreateObjectMaterial(
		std::string tag,
		glm::vec3 diffuseColor,
		glm::vec3 specularColor,
		float shininess)
	{
		OBJECT_MATERIAL material;
		material.diffuseColor = diffuseColor;
		material.specularColor = specularColor;
		material.shininess = shininess;
		material.tag = tag;

		return material;
	}
}


/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}   

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	//Here I am loading in the textures I will be using 
	// Enhancement: I replaced repeated texture loading calls with a loop and an array of texture information. 
	// This makes it easier to manage and add new textures to my scene in the future.

	struct TextureInfo
	{
		const char* filePath;
		std::string tag;
	};

	TextureInfo textures[] =
	{
		{ "textures/pavers.jpg", "ground" },
		{ "textures/bricks.jpg", "brick" },
		{ "textures/wicker.png", "wicker" },
		{ "textures/scratched_wood.jpg", "scratched_wood" },
		{ "textures/gold-seamless-texture.jpg", "gold" },
		{ "textures/stainless.jpg", "stainless" },
		{ "textures/stainedglass.jpg", "stained_glass" },
		{ "textures/porcelain.jpg", "porcelain" },
		{ "textures/coffee.jpg", "coffee" },
		{ "textures/marble_counter.png", "counter" },
		{ "textures/branches.jpg", "branches" },
		{ "textures/tiles.jpg", "tiles" }
	};

	for (const TextureInfo& texture : textures)
	{
		CreateGLTexture(texture.filePath, texture.tag);
	}

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	//Enhancement: I replaced repeated code with a reusable
	// helper method. This keeps my material list shorter and easier to update.
	m_objectMaterials.push_back(CreateObjectMaterial(
		"metal",
		glm::vec3(0.4f, 0.4f, 0.4f),
		glm::vec3(0.7f, 0.7f, 0.6f),
		52.0f));

	m_objectMaterials.push_back(CreateObjectMaterial(
		"wood",
		glm::vec3(0.2f, 0.2f, 0.3f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		0.1f));

	m_objectMaterials.push_back(CreateObjectMaterial(
		"glass",
		glm::vec3(0.2f, 0.2f, 0.2f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		95.0f));

	m_objectMaterials.push_back(CreateObjectMaterial(
		"plate",
		glm::vec3(0.4f, 0.4f, 0.4f),
		glm::vec3(0.2f, 0.2f, 0.2f),
		64.0f));

	m_objectMaterials.push_back(CreateObjectMaterial(
		"backdrop",
		glm::vec3(0.8f, 0.8f, 0.9f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		2.0f));

	m_objectMaterials.push_back(CreateObjectMaterial(
		"coffee",
		glm::vec3(0.18f, 0.11f, 0.06f),
		glm::vec3(0.90f, 0.80f, 0.70f),
		128.0f));
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.25f, -1.0f, -0.25f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.16f, 0.15f, 0.13f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.92f, 0.88f, 0.78f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.15f, 0.13f, 0.10f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -6.0f, 9.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.04f, 0.035f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.18f, 0.16f, 0.13f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.05f, 0.045f, 0.04f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	// point light 2 
	// Enhancement: I fixed an error where the position of the second point light was the same as the first point light. 
	// I changed the position to be different so that both lights can contribute to the scene lighting.
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.04f, 0.035f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.18f, 0.16f, 0.13f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.05f, 0.045f, 0.04f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	// point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.12f, 0.11f, 0.09f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.10f, 0.09f, 0.08f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 3.8f, 3.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.10f, 0.09f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.08f, 0.07f, 0.06f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);
	
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.10f, 0.09f, 0.07f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 0.35f, 0.30f, 0.22f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.08f, 0.07f, 0.06f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(42.5f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(48.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);
	m_pShaderManager->setVec3Value("spotLight.position", 0.0f, 8.0f, 8.0f);
	m_pShaderManager->setVec3Value("spotLight.direction", 0.0f, -1.0f, -1.0f);

}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
    // in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// Enhancement: I added a structure for defining the properties of each scene object and a helper method for rendering them.
	enum MeshType
	{
		PLANE_MESH,
		CYLINDER_MESH,
		TORUS_MESH,
		BOX_MESH
	};

	struct SceneObject
	{
		MeshType meshType;
		glm::vec3 scale;
		float xRotation;
		float yRotation;
		float zRotation;
		glm::vec3 position;
		bool useTexture;
		std::string textureTag;
		glm::vec2 uvScale;
		glm::vec4 color;
		std::string materialTag;
	};

	auto RenderSceneObject = [&](const SceneObject& object)
		{
			SetTransformations(
				object.scale,
				object.xRotation,
				object.yRotation,
				object.zRotation,
				object.position);

			if (object.useTexture)
			{
				SetShaderTexture(object.textureTag);
				SetTextureUVScale(object.uvScale.x, object.uvScale.y);
			}
			else
			{
				SetShaderColor(object.color.r, object.color.g, object.color.b, object.color.a);
			}

			SetShaderMaterial(object.materialTag);

			switch (object.meshType)
			{
			case PLANE_MESH:
				m_basicMeshes->DrawPlaneMesh();
				break;
			case CYLINDER_MESH:
				m_basicMeshes->DrawCylinderMesh();
				break;
			case TORUS_MESH:
				m_basicMeshes->DrawTorusMesh();
				break;
			case BOX_MESH:
				m_basicMeshes->DrawBoxMesh();
				break;
			}
		};

	// Enhancement: I fixed an issue with the original code where the transformations, texture, material,
	// and draw calls were repeated for each object.
	std::vector<SceneObject> sceneObjects =
	{
		// Base plane
		{ PLANE_MESH, glm::vec3(20.0f, 1.0f, 10.0f), 0.0f, 0.0f, 0.0f,
		  glm::vec3(0.0f, 0.0f, 0.0f), true, "branches", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.0f), "wood" },

	   //Start Milestone Two
	   //Mug One 
	   // Here I am creating my mugs base using a cylinder
	   { CYLINDER_MESH, glm::vec3(1.0f, 2.0f, 1.0f), 0.0f, -25.0f, 0.0f,
	     glm::vec3(2.5f, 0.53f, 3.5f), true, "porcelain", glm::vec2(1.0f, 1.0f),
		 glm::vec4(0.0f), "plate" },

	   //Mug Two 
	   { CYLINDER_MESH, glm::vec3(1.0f, 2.0f, 1.0f), 0.0f, -25.0f, 0.0f,
	     glm::vec3(-0.4f, 0.53f, 3.5f), true, "porcelain", glm::vec2(1.0f, 1.0f),
	     glm::vec4(0.0f), "plate" },

		//Here I am creating the handle for mug one using a torus shape.
		{ TORUS_MESH, glm::vec3(0.55f, 0.55f, 0.55f), 0.0f, 42.0f, 0.0f,
		  glm::vec3(1.55f, 1.38f, 4.05f), true, "porcelain", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.0f), "plate" },

		// Handle for Mug Two
		{ TORUS_MESH, glm::vec3(0.55f, 0.55f, 0.55f), 0.0f, 42.0f, 0.0f,
		  glm::vec3(-1.35f, 1.38f, 4.05f), true, "porcelain", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.0f), "plate" },
		//End Milestone Two

		//Begin Milestone Three
		//here I added a background plane 
		{ PLANE_MESH, glm::vec3(20.0f, 1.0f, 10.0f), 90.0f, 0.0f, 0.0f,
		  glm::vec3(0.0f, 9.0f, -10.0f), true, "branches", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.0f), "backdrop" },

		// I added another plane this plane will be the counter top 
		// the shapes are resting on 
		{ PLANE_MESH, glm::vec3(11.5f, 1.0f, 4.5f), 0.0f, 30.0f, 0.0f,
		  glm::vec3(4.5f, 0.5f, 3.0f), true, "tiles", glm::vec2(3.0f, 3.0f),
		  glm::vec4(0.0f), "plate" },

		/// Book
		// Bottom cover
	    { BOX_MESH, glm::vec3(2.60f, 0.10f, 1.85f), 0.0f, 25.0f, 0.0f,
		  glm::vec3(5.55f, 0.53f, 2.28f), false, "", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.55f, 0.08f, 0.08f, 1.0f), "plate" },

		//Pages
		{ BOX_MESH, glm::vec3(2.48f, 0.10f, 1.72f), 0.0f, 25.0f, 0.0f,
		  glm::vec3(5.55f, 0.69f, 2.28f), false, "", glm::vec2(1.0f, 1.0f),
		  glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), "plate" },

		//Top cover
		{ BOX_MESH, glm::vec3(2.60f, 0.10f, 1.85f), 0.0f, 25.0f, 0.0f,
		  glm::vec3(5.55f, 0.79f, 2.28f), false, "", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.55f, 0.08f, 0.08f, 1.0f), "plate" },

		// Spine 
		{ BOX_MESH, glm::vec3(2.60f, 0.26f, 0.18f), 0.0f, 25.0f, 0.0f,
		  glm::vec3(5.90f, 0.66f, 3.04f), false, "", glm::vec2(1.0f, 1.0f),
		  glm::vec4(0.50f, 0.07f, 0.07f, 1.0f), "plate" }
	};

	for (const SceneObject& object : sceneObjects)
	{
		RenderSceneObject(object);
	}

	// Coffee in Mug One
	// Here I am creating my coffee in mug 1 using a thin cylinder
	scaleXYZ = glm::vec3(0.85f, 0.03f, 0.85f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.5f, 2.51f, 3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// coffee color
	//SetShaderColor(0.36f, 0.22f, 0.10f, 1.0f);
	SetShaderTexture("coffee");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("coffee");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	// Coffee in Mug Two
	scaleXYZ = glm::vec3(0.85f, 0.03f, 0.85f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.4f, 2.51f, 3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("coffee");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("coffee");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	// Here I am creating a coaster using a thin cylinder 
	scaleXYZ = glm::vec3(1.40f, 0.04f, 1.40f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.5f, 0.5f, 3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("scratched_wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	// Coaster Two
	scaleXYZ = glm::vec3(1.40f, 0.04f, 1.40f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.4f, 0.5f, 3.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("scratched_wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	m_basicMeshes->DrawCylinderMesh(true, false, false);
}