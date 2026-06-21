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
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/pavers.jpg",
		"ground");

	bReturn = CreateGLTexture(
		"textures/bricks.jpg",
		"brick");

	bReturn = CreateGLTexture(
		"textures/wicker.png",
		"wicker");

	bReturn = CreateGLTexture(
		"textures/scratched_wood.jpg",
		"scratched_wood");

	bReturn = CreateGLTexture(
		"textures/gold-seamless-texture.jpg",
		"gold");

	bReturn = CreateGLTexture(
		"textures/stainless.jpg",
		"stainless");

	bReturn = CreateGLTexture(
		"textures/stainedglass.jpg",
		"stained_glass");

	bReturn = CreateGLTexture(
		"textures/porcelain.jpg",
		"porcelain");

	bReturn = CreateGLTexture(
		"textures/coffee.jpg",
		"coffee");

	bReturn = CreateGLTexture(
		"textures/marble_counter.png",
		"counter");

	bReturn = CreateGLTexture(
		"textures/branches.jpg",
		"branches");

	bReturn = CreateGLTexture(
		"textures/tiles.jpg",
		"tiles");

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
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	goldMaterial.shininess = 52.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL plateMaterial;
	plateMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plateMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plateMaterial.shininess = 64.0;
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 2.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL coffeeMaterial;
	coffeeMaterial.diffuseColor = glm::vec3(0.18f, 0.11f, 0.06f);   
	coffeeMaterial.specularColor = glm::vec3(0.90f, 0.80f, 0.70f);   
	coffeeMaterial.shininess = 128.0f;                           
	coffeeMaterial.tag = "coffee";

	m_objectMaterials.push_back(coffeeMaterial);
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
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.04f, 0.035f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.18f, 0.16f, 0.13f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.05f, 0.045f, 0.04f);
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
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.333, 0.420, 0.184, 1);
	SetShaderTexture("branches");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	//Start Milestone Two
	//Mug One 
	// Here I am creating my mugs base using a cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.5f, 0.53f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.000, 1.000, 0.941, 1);
	SetShaderTexture("porcelain");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plate");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();// set the XYZ scale for the mesh

	//Mug Two 
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = -25.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.4f, 0.53f, 3.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderTexture("porcelain");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plate");

	m_basicMeshes->DrawCylinderMesh();

	//Coffee in Mug One
	// Here I am creating my coffee in mug 1 using a thin cylinder
	// set the XYZ scale for the mesh
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

	//Here I am creating the handle for mug one using a torus shape.
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.55f, 0.55f, 0.55f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 42.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.55f, 1.38f, 4.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.000, 1.000, 0.941, 1);
	SetShaderTexture("porcelain");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plate");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();// set the XYZ scale for the mesh

	// Handle for Mug Two
	scaleXYZ = glm::vec3(0.55f, 0.55f, 0.55f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 42.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-1.35f, 1.38f, 4.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderTexture("porcelain");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plate");

	m_basicMeshes->DrawTorusMesh();
	//End Milestone Two

	//Begin Milestone Three
	//here I added a background plane 
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.333, 0.420, 0.184, 1);
	SetShaderTexture("branches");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("backdrop");

	m_basicMeshes->DrawPlaneMesh();

	// I added another plane this plane will be the counter top 
	// the shapes are resting on 
	scaleXYZ = glm::vec3(11.5f, 1.0f, 4.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(4.5f, 0.5f, 3.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.78f, 0.78f, 0.78f, 1.0f);
	SetShaderTexture("tiles");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("plate");

	m_basicMeshes->DrawPlaneMesh();

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

	/// Book
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// Bottom cover
	scaleXYZ = glm::vec3(2.60f, 0.10f, 1.85f);
	positionXYZ = glm::vec3(5.55f, 0.53f, 2.28f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.55f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("plate");
	m_basicMeshes->DrawBoxMesh();

	//Pages
	scaleXYZ = glm::vec3(2.48f, 0.10f, 1.72f);
	positionXYZ = glm::vec3(5.55f, 0.69f, 2.28f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plate");
	m_basicMeshes->DrawBoxMesh();

	//Top cover
	scaleXYZ = glm::vec3(2.60f, 0.10f, 1.85f);
	positionXYZ = glm::vec3(5.55f, 0.79f, 2.28f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.55f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("plate");
	m_basicMeshes->DrawBoxMesh();

	// Spine 
	scaleXYZ = glm::vec3(2.60f, 0.26f, 0.18f);

	positionXYZ = glm::vec3(5.90f, 0.66f, 3.04f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.50f, 0.07f, 0.07f, 1.0f);
	SetShaderMaterial("plate");
	m_basicMeshes->DrawBoxMesh();
}