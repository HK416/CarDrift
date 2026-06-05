#include "stdafx.h"
#include "SkyboxObject.h"

SkyboxObject::SkyboxObject(Mesh* cubeMesh, Material* skyboxMaterial) 
    : MeshObject(cubeMesh, {skyboxMaterial}) {}
