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
    std::map<std::string, BoneInfo> m_BoneInfoMap;
} Animation;


std::vector<Animation> Animations;


Animation* CreateAnimation(const std::string& animationPath, unsigned int ModelId)
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

    ReadMissingBones(&anim, animation, ModelId);

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

void ReadMissingBones(Animation* anim, const aiAnimation* animation, unsigned int ModelId)
{
    int size = animation->mNumChannels;

    auto& boneInfoMap = GetBoneInfoMap(ModelId); // getting m_BoneInfoMap from Model class
    int& boneCount = GetBoneCount(ModelId); // getting the m_BoneCounter from Model class

    // reading channels(bones engaged in an animation and their keyframes)
    for (int i = 0; i < size; i++) {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }

        Bone bone = CreateBone(channel->mNodeName.data, boneInfoMap[channel->mNodeName.data].id, channel);

        m_Bones.push_back(bone);
    }

    anim->m_BoneInfoMap = boneInfoMap;
}

void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++) {
        AssimpNodeData newData;
        ReadHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}