#pragma once

#include <assimp/scene.h>
#include <functional>
#include <glm/glm.hpp>
// #include <animdata.h>
#include <bone.h>
#include <map>
#include <model_animation.h>
#include <vector>

struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

// previously private var
//float m_Duration;
//int m_TicksPerSecond;
std::vector<Bone> m_Bones;
//AssimpNodeData m_RootNode;
//std::map<std::string, BoneInfo> m_BoneInfoMap;


typedef struct {
    float m_Duration;
    int m_TicksPerSecond;
    AssimpNodeData m_RootNode;
} Animation;


std::vector<Animation> Animations;


void ReadMissingBones(Animation* anim, const aiAnimation* animation, Model* model);
void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);


Animation* CreateAnimation(const std::string& animationPath, Model* model)
{
    Animation anim;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);


    auto animation = scene->mAnimations[0];


    anim.m_Duration = animation->mDuration;
    anim.m_TicksPerSecond = animation->mTicksPerSecond;
    
    aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;

    globalTransformation = globalTransformation.Inverse();

    ReadHierarchyData(anim.m_RootNode, scene->mRootNode);

    ReadMissingBones(&anim, animation, model);

    return &anim;
}

Bone* FindBone(const std::string& name)
{
    auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
        [&](const Bone& Bone) {
            return Bone.m_Name == name;
        });
    if (iter == m_Bones.end())
        return nullptr;
    else
        return &(*iter);
}


void ReadHierarchyData(AssimpNodeData& dest, const aiNode* node)
{
    assert(node);

    dest.name = node->mName.data;

    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);

    dest.childrenCount = node->mNumChildren;

    for (int i = 0; i < node->mNumChildren; i++) {

        AssimpNodeData newData;

        ReadHierarchyData(newData, node->mChildren[i]);

        dest.children.push_back(newData);

    }
}