#pragma once

typedef SceneObject;
typedef Matrix4x4;

typedef struct {
    string nodeName;    
    bool bone;
} skeletonNode;

vector<skeletonNode> skeletonNodes;

typedef struct {
    int mNumMeshes;
} aiNode;


typedef struct {
    int mNumMeshes;
} MeshNode;

MeshNode rootNode;

MeshNode* createMeshNode();
void addChild(MeshNode rootNode, MeshNode* childNode);
void CopyMeshes(aiNode sourceNode, MeshNode* targetNode)

void recurseIterate(aiNode node, string name, string MeshNodeName, string MeshNodeParent)
{
    if node
        .name = name;
    skeletoneNodes.push_back(name, 1);

    if node.parent = MeshNodeName || MeshNodeParent: return;

    recurseIterate(node.parent, name, meshnodename, meshnodeparent);
    
}

void CopyNodesWithMeshes(aiNode node, MeshNode rootNode, Matrix4x4 accTransform)
{
    SceneObject parent;
    Matrix4x4 transform;

    // if node has meshes, create a new scene object for it
    if (node.mNumMeshes > 0) {
       
        MeshNode* meshNode = createMeshNode();

        addChild(rootNode, meshNode);

        // copy the meshes
        CopyMeshes(node, meshNode);

        foreach(meshId in meshNode) {
            currentMesh = scen.meshes[meshID];
            bones = currentMesh.Bones;

            recurseIterate(node, bone.name, meshNode.name, meshNode.parent.name)
        }

        // the new object is the parent for all child nodes
        parent = meshNode;
        transform.SetUnity();
    } else {
        // if no meshes, skip the node, but keep its transformation
        parent = targetParent;
        transform = node.mTransformation * accTransform;
    }
    // continue for all child nodes
    for (all node.mChildren)
        CopyNodesWithMeshes(node.mChildren[a], parent, transform);
}

void CreateSkeleton(aiNode node) {

    foreach skeleonNodes 
        if skeletonNode.name == node.anme && skeletonNode.bool = 1
            
}