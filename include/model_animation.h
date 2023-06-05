#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <mesh.h>
#include <shader_m.h>

#include <fstream>
#include <iostream>
// #include <animdata.h>
#include <assimp_glm_helpers.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// model data
vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

string directory;
bool gammaCorrection = false;

struct BoneInfo {
    /*id is index in finalBoneMatrices*/
    int id;

    /*offset matrix transforms vertex from model space to bone space*/
    glm::mat4 offset;
};

typedef struct {
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    vector<Mesh*> meshes;
    int m_BoneCounter;
} Model;

Model currentModel;

vector<Model*> models;

/*
std::map<string, BoneInfo> m_BoneInfoMap;
int m_BoneCounter = 0;
/*
// constructor, expects a filepath to a 3D model.
/*
Model(string const& path, bool gamma = false)
        : gammaCorrection(gamma)
{
        loadModel(path);
}
*/

void DrawModel(Model* model, unsigned int shaderID);
Model* LoadModel(string const& path);
void processNode(aiNode* node, const aiScene* scene);
Mesh* processMesh(aiMesh* mesh, const aiScene* scene);
void SetVertexBoneData(VertexData& vertexData, int boneID, float weight);
void ExtractBoneWeightForVertices(std::vector<VertexData>& vertices, aiMesh* mesh, const aiScene* scene);
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

// draws the model, and thus all its meshes
void DrawModel(Model* model, unsigned int shaderID)
{
    for (unsigned int i = 0; i < model->meshes.size(); i++)
        DrawMesh(model->meshes[i], shaderID);
}

// auto& GetBoneInfoMap() { return m_BoneInfoMap; }
// int& GetBoneCount() { return m_BoneCounter; }

// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
Model* LoadModel(string const& path)
{
    Model* model;
    currentModel = *model;
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        return model;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('\\'));

    //process meshes

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);

    return model;
}

// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
void processNode(aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        currentModel.meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

// get assimp mesh data then load mesh data onto gpu
Mesh* processMesh(aiMesh* mesh, const aiScene* scene)
{
    vector<VertexData> vertexDataVec;
    vector<unsigned int> indices;
    vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        VertexData vertexData;
        vertexData.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
        vertexData.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertexData.TexCoords = vec;
        } else
            vertexData.TexCoords = glm::vec2(0.0f, 0.0f);

        vertexDataVec.push_back(vertexData);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    ExtractBoneWeightForVertices(vertexDataVec, mesh, scene);

    Mesh* meshPointer = LoadMeshVertexData(vertexDataVec, indices, textures);

    return meshPointer;
}

void SetVertexBoneData(VertexData& vertexData, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if (vertexData.m_BoneIDs[i] < 0) {
            vertexData.m_Weights[i] = weight;
            vertexData.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

void ExtractBoneWeightForVertices(std::vector<VertexData>& vertexData, aiMesh* mesh, const aiScene* scene)
{
    auto& boneInfoMap = currentModel.m_BoneInfoMap;
    int& boneCount = currentModel.m_BoneCounter;

    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {

        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {

            BoneInfo newBoneInfo;
            newBoneInfo.id = boneCount;
            newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCount;
            boneCount++;

        } else {
            boneID = boneInfoMap[boneName].id;
        }

        assert(boneID != -1);

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {

            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;

            assert(vertexId <= vertexData.size());

            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
            {
                if (vertexData[vertexId].m_BoneIDs[i] < 0) {
                    vertexData[vertexId].m_Weights[i] = weight;
                    vertexData[vertexId].m_BoneIDs[i] = boneID;
                    break;
                }
            }
        }
    }
}

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip) { // if texture hasn't been loaded already, load it
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
        }
    }
    return textures;
}

#endif