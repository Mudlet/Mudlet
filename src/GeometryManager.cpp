/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "GeometryManager.h"
#include "ResourceManager.h"

#include <QDebug>
#include <QFile>
#include <QImage>

GeometryManager::GeometryManager()
{
}

GeometryManager::~GeometryManager()
{
    cleanup();
}

void GeometryManager::initialize()
{
    if (mInitialized) {
        return;
    }

    initializeOpenGLFunctions();

    // Get function pointers for instancing (OpenGL 3.3+)
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context) {
        glVertexAttribDivisor = reinterpret_cast<PFNGLVERTEXATTRIBDIVISORPROC>(context->getProcAddress("glVertexAttribDivisor"));
        glDrawElementsInstanced = reinterpret_cast<PFNGLDRAWELEMENTSINSTANCEDPROC>(context->getProcAddress("glDrawElementsInstanced"));
    }

    generateCubeTemplate();
    mInitialized = true;
}

void GeometryManager::cleanup()
{
    if (mPlayerIconTemplate.has_value()) {
        mPlayerIconTemplate->clearTexture();
    }

    mCubeTemplate.clear();
    mPlayerIconTemplate.reset();
    mInitialized = false;
}

void GeometryManager::generateCubeTemplate()
{
    // Generate unit cube centered at origin using indexed geometry
    mCubeTemplate.clear();

    // Define 24 unique vertices + normals for a unit cube
    // Vertex order: front face (counter-clockwise from bottom-left), then back face
    // Front bottom-left
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 0: Front bottom-left (normal:front)
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << 0.0 << -1.0 << 0.0;  // 1: Front bottom-left (normal:bottom)
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << -1.0 << 0.0 << 0.0;  // 2: Front bottom-left (normal:left)
    // Front bottom-right
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 3: Front bottom-right (normal:front)
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 0.0 << -1.0 << 0.0;  // 4: Front bottom-right (normal:bottom)
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 1.0 << 0.0 << 0.0;   // 5: Front bottom-right (normal:right)
    // Front top-right
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 6: Front top-right (normal:front)
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 0.0 << 1.0 << 0.0;   // 7: Front top-right (normal:top)
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 1.0 << 0.0 << 0.0;   // 8: Front top-right (normal:right)
    // Front top-left
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 9: Front top-left (normal:front)
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << 0.0 << 1.0 << 0.0;   // 10: Front top-left (normal:top)
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << -1.0 << 0.0 << 0.0;  // 11: Front top-left (normal:left)
    // Back bottom-left
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 12: Back bottom-left (normal:back)
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << 0.0 << -1.0 << 0.0;  // 13: Back bottom-left (normal:bottom)
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << -1.0 << 0.0 << 0.0;  // 14: Back bottom-left (normal:left)
    // Back bottom-right
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 15: Back bottom-right (normal:back)
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 0.0 << -1.0 << 0.0;  // 16: Back bottom-right (normal:bottom)
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 1.0 << 0.0 << 0.0;   // 17: Back bottom-right (normal:right)
    // Back top-right
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 18: Back top-right (normal:back)
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 0.0 << 1.0 << 0.0;   // 19: Back top-right (normal:top)
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 1.0 << 0.0 << 0.0;   // 20: Back top-right (normal:right)
    // Back top-left
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 21: Back top-left (normal:back)
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << 0.0 << 1.0 << 0.0;   // 22: Back top-left (normal:top)
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << -1.0 << 0.0 << 0.0;  // 23: Back top-left (normal:left)

    // Define indices for the 12 triangles (6 faces × 2 triangles each)
    // Counter-clockwise winding order for front-facing triangles
    QVector<unsigned int> indices = {
        // Front face
        0, 3, 6,  0, 6, 9,
        // Back face
        12, 15, 18,  12, 18, 21,
        // Left face
        2, 11, 23,  2, 23, 14,
        // Right face
        5, 8, 20,  5, 20, 17,
        // Bottom face
        1, 4, 16,  1, 13, 16,
        // Top face
        7, 10, 22,  7, 22, 19,
    };

    mCubeTemplate.indices = indices;

    // Colors will be set per instance, so we don't populate them in the template
}

GeometryData GeometryManager::transformCubeTemplate(QMatrix4x4 transform, float r, float g, float b, float a)
{
    GeometryData result;

    // Transform vertices and copy normals
    for (int i = 0; i < mCubeTemplate.vertices.size(); i += 6) {
        // Scale and translate vertex
        QVector3D vertex = QVector3D(mCubeTemplate.vertices[i], mCubeTemplate.vertices[i+1], mCubeTemplate.vertices[i+2]);
        vertex = transform.map(vertex);
        result.vertices << vertex.x();
        result.vertices << vertex.y();
        result.vertices << vertex.z();

        // Copy normal (no transformation needed since it's a uniform scale)
        vertex = QVector3D(mCubeTemplate.vertices[i+3], mCubeTemplate.vertices[i+4], mCubeTemplate.vertices[i+5]);
        vertex = transform.map(vertex) - transform.map(QVector3D());
        vertex.normalize();
        result.vertices << vertex.x();
        result.vertices << vertex.y();
        result.vertices << vertex.z();

        // Set color for this vertex
        result.colors << r << g << b << a;
    }

    // Copy indices (they don't need transformation)
    result.indices = mCubeTemplate.indices;

    return result;
}

GeometryData GeometryManager::generateCubeGeometry(float x, float y, float z, float size, float r, float g, float b, float a)
{
    if (!mInitialized) {
        qWarning() << "GeometryManager: generateCubeGeometry called before initialize()";
        return GeometryData();
    }

    QMatrix4x4 transform = QMatrix4x4();
    transform.translate(x, y, z);
    transform.scale(size);
    return transformCubeTemplate(transform, r, g, b, a);
}

GeometryData GeometryManager::generateLineGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return GeometryData();
    }

    GeometryData result;
    result.vertices = vertices;
    result.colors = colors;

    return result;
}

GeometryData GeometryManager::generateTriangleGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty() || vertices.size() % 3 != 0 || colors.size() % 4 != 0) {
        qDebug() << "GeometryManager: Invalid vertex or color array size";
        return GeometryData();
    }

    // Check that we have the right ratio: 3 floats per vertex, 4 floats per color
    if (vertices.size() / 3 != colors.size() / 4) {
        qDebug() << "GeometryManager: Vertex count doesn't match color count";
        return GeometryData();
    }

    GeometryData result;
    //result.vertices = vertices;
    result.colors = colors;

    // add normals pointing up
    for (int i=0; i < vertices.size(); i+=3) {
        result.vertices << vertices[i] << vertices[i+1] << vertices[i+2] << 0.0f << 0.0f << 1.0f;
    }

    return result;
}

GeometryData GeometryManager::generatePlayerIconGeometry(float scale, float rotX, float rotY, float rotZ)
{
    // Check if we need to regenerate the template
    static float lastScale = -1.0f;
    static float lastRotX = -999.0f;
    static float lastRotY = -999.0f;
    static float lastRotZ = -999.0f;

    bool parametersChanged = (scale != lastScale || rotX != lastRotX || rotY != lastRotY || rotZ != lastRotZ);

    // Only regenerate if parameters changed or template doesn't exist
    if (parametersChanged || !mPlayerIconTemplate.has_value()) {
        loadPlayerIconTemplate(scale, rotX, rotY, rotZ);
        lastScale = scale;
        lastRotX = rotX;
        lastRotY = rotY;
        lastRotZ = rotZ;
    }

    return mPlayerIconTemplate.value_or(GeometryData{});
}

void GeometryManager::clearPlayerIconTemplate()
{
    mPlayerIconTemplate.reset();
}

void GeometryManager::loadPlayerIconTemplate(float scale, float rotX, float rotY, float rotZ)
{
    GeometryData result;
    QFile file(":/3d-models/sword/sword.glb");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GeometryManager: Failed to open sword GLB model file";
        mPlayerIconTemplate = result;
        return;
    }

    QByteArray modelData = file.readAll();
    file.close();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(modelData.constData(), modelData.size(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qWarning() << "GeometryManager: Failed to load sword model:" << importer.GetErrorString();
        mPlayerIconTemplate = result;
        return;
    }

    // Process the first mesh in model
    if (scene->mNumMeshes > 0) {
        const aiMesh* mesh = scene->mMeshes[0];

        // Create transformation matrices for the additional rotations
        QMatrix4x4 rotationX;
        rotationX.rotate(rotX, 1.0f, 0.0f, 0.0f);

        QMatrix4x4 rotationY;
        rotationY.rotate(rotY, 0.0f, 1.0f, 0.0f);

        QMatrix4x4 rotationZ;
        rotationZ.rotate(rotZ, 0.0f, 0.0f, 1.0f);

        // Base rotation to make sword point upward (90 degrees around Z)
        QMatrix4x4 baseRotation;
        baseRotation.rotate(90.0f, 0.0f, 0.0f, 1.0f);

        // Combined transformation: scale * user rotations * base rotation
        QMatrix4x4 combinedTransform = rotationZ * rotationY * rotationX;

        // Extract vertices, normals, and texture coordinates
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            // Original vertex
            QVector3D vertex(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Apply base rotation first (90-degree rotation around Z-axis to make sword point upward)
            QVector3D rotatedVertex(-vertex.y(), vertex.x(), vertex.z());

            // Apply user rotations
            rotatedVertex = combinedTransform.map(rotatedVertex);

            // Apply scaling
            rotatedVertex *= scale;

            result.vertices << rotatedVertex.x() << rotatedVertex.y() << rotatedVertex.z();

            // Apply same transformations to normals (normals shouldn't be scaled)
            if (mesh->HasNormals()) {
                QVector3D normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                // Apply base rotation
                QVector3D rotatedNormal(-normal.y(), normal.x(), normal.z());
                // Apply user rotations
                rotatedNormal = combinedTransform.map(rotatedNormal);

                result.normals << rotatedNormal.x() << rotatedNormal.y() << rotatedNormal.z();
            } else {
                result.normals << 0.0f << 0.0f << 1.0f;
            }

            // Extract texture coordinates if available
            if (mesh->mTextureCoords[0]) {
                float u = mesh->mTextureCoords[0][i].x;
                float v = 1.0f - mesh->mTextureCoords[0][i].y; // Flip V coordinate for OpenGL
                result.textureCoords << u << v;

                // UV coordinates processed
            } else {
                result.textureCoords << 0.0f << 0.0f;
            }

            // Set color to white for textured rendering (texture will provide color)
            result.colors << 1.0f << 1.0f << 1.0f << 1.0f;
        }

        // Extract indices - ensure we have triangulated faces
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices == 3) { // Only process triangles
                result.indices.append(face.mIndices[0]);
                result.indices.append(face.mIndices[1]);
                result.indices.append(face.mIndices[2]);
            } else {
                qWarning() << "GeometryManager: Non-triangular face found with" << face.mNumIndices << "indices";
            }
        }

        // Process material textures
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Check for diffuse/base color texture
            aiTextureType textureType = aiTextureType_DIFFUSE;
            if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
                textureType = aiTextureType_BASE_COLOR;
            }

            if (material->GetTextureCount(textureType) > 0) {
                aiString texturePath;
                material->GetTexture(textureType, 0, &texturePath);
                // Processing texture reference

                // Check if it's an embedded texture reference (format: *0, *1, etc.)
                QString texturePathStr = QString::fromStdString(texturePath.C_Str());
                if (texturePathStr.startsWith("*")) {
                    bool ok;
                    int textureIndex = texturePathStr.mid(1).toInt(&ok);
                    if (ok && textureIndex >= 0 && textureIndex < static_cast<int>(scene->mNumTextures)) {
                        // Using embedded texture
                        // We'll use this index below instead of hardcoded 0
                    }
                }
            }
        }

        // Load PBR textures if available
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Helper function to load a texture by type
            auto loadTextureByType = [&](aiTextureType textureType, unsigned int& textureId, const QString& typeName) {
                if (material->GetTextureCount(textureType) > 0) {
                    aiString texturePath;
                    aiTextureMapping mapping;
                    unsigned int uvIndex = 0;
                    ai_real blend;
                    aiTextureOp op;
                    aiTextureMapMode mapMode;

                    material->GetTexture(textureType, 0, &texturePath, &mapping, &uvIndex, &blend, &op, &mapMode);
                    // Texture uses UV channel

                    QString texturePathStr = QString::fromStdString(texturePath.C_Str());

                    if (texturePathStr.startsWith("*")) {
                        bool ok;
                        int materialTextureIndex = texturePathStr.mid(1).toInt(&ok);
                        if (ok && materialTextureIndex >= 0 && materialTextureIndex < static_cast<int>(scene->mNumTextures)) {
                            // Loading texture from index

                            const aiTexture* texture = scene->mTextures[materialTextureIndex];

                            // Create OpenGL texture
                            GLuint newTextureId;
                            glGenTextures(1, &newTextureId);
                            glBindTexture(GL_TEXTURE_2D, newTextureId);

                            // Create QImage from embedded texture data
                            QImage image;
                            if (texture->mHeight == 0) {
                                // Compressed texture data
                                QByteArray textureData(reinterpret_cast<const char*>(texture->pcData), texture->mWidth);
                                image = QImage::fromData(textureData);
                            } else {
                                // Uncompressed texture data (RGBA)
                                const unsigned char* data = reinterpret_cast<const unsigned char*>(texture->pcData);
                                image = QImage(data, texture->mWidth, texture->mHeight, QImage::Format_RGBA8888);
                            }

                            if (!image.isNull()) {
                                // Convert to OpenGL format and upload
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
#else
                                // Deprecated in 6.9 and due for removal in 6.13:
                                QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
#endif
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.constBits());

                                // Set texture parameters based on texture type
                                if (typeName == "base color") {
                                    // Base color textures benefit from mipmapping for distance
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                    glGenerateMipmap(GL_TEXTURE_2D);
                                } else if (typeName == "normal") {
                                    // Normal maps should use linear filtering, no mipmaps to preserve detail
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                } else {
                                    // Metallic/roughness and other maps
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                }
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                                // Set texture filtering

                                textureId = newTextureId;
                                // Texture loaded successfully
                            } else {
                                qWarning() << "GeometryManager: Failed to load" << typeName << "texture data";
                                glDeleteTextures(1, &newTextureId);
                            }
                        }
                    }
                }
            };

            // Load all PBR texture types
            loadTextureByType(aiTextureType_BASE_COLOR, result.baseColorTextureId, "base color");
            if (result.baseColorTextureId == 0) {
                loadTextureByType(aiTextureType_DIFFUSE, result.baseColorTextureId, "diffuse");
            }
            loadTextureByType(aiTextureType_METALNESS, result.metallicRoughnessTextureId, "metallic roughness");
            loadTextureByType(aiTextureType_NORMALS, result.normalTextureId, "normal");

            // Extract PBR material factors
            aiColor4D baseColor;
            if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
                result.baseColorFactor[0] = baseColor.r;
                result.baseColorFactor[1] = baseColor.g;
                result.baseColorFactor[2] = baseColor.b;
                result.baseColorFactor[3] = baseColor.a;
            }

            material->Get(AI_MATKEY_METALLIC_FACTOR, result.metallicFactor);

            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, result.roughnessFactor);

            // Check for emissive factor (material self-illumination)
            aiColor3D emissive;
            material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);

            // Check for opacity/transparency
            float opacity = 1.0f;
            material->Get(AI_MATKEY_OPACITY, opacity);

            // Set legacy textureId for backward compatibility with non-PBR rendering
            result.textureId = result.baseColorTextureId;
        }
    }

    qDebug() << "Loaded model" << file.fileName();

    mPlayerIconTemplate = std::move(result);
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   QOpenGLBuffer& indexBuffer,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Upload vertex data (cached)
    vertexBuffer.bind();
    if (!geometry.verticesUploaded) {
        vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
        geometry.verticesUploaded = true;
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload color data (cached)
    colorBuffer.bind();
    if (!geometry.colorsUploaded) {
        colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
        geometry.colorsUploaded = true;
    }
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    // Upload normal data (cached)
    normalBuffer.bind();
    if (!geometry.normalsUploaded) {
        normalBuffer.allocate(geometry.normals.data(), geometry.normals.size() * sizeof(float));
        geometry.normalsUploaded = true;
    }
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);

    // Draw the geometry - use indexed rendering if indices are available
    if (geometry.hasIndices()) {
        // Upload index data (cached)
        indexBuffer.bind();
        if (!geometry.indicesUploaded) {
            indexBuffer.allocate(geometry.indices.data(), geometry.indices.size() * sizeof(unsigned int));
            geometry.indicesUploaded = true;
        }

        // Draw using indices
        glDrawElements(drawMode, geometry.indexCount(), GL_UNSIGNED_INT, nullptr);
    } else {
        // Draw using vertex arrays (for flat triangles)
        glDrawArrays(drawMode, 0, geometry.vertexCount());
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   QOpenGLBuffer& indexBuffer,
                                   ResourceManager* resourceManager,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    // Call the original render method
    renderGeometry(geometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode);

    // Track draw call statistics
    if (resourceManager) {
        if (geometry.hasIndices()) {
            resourceManager->onDrawCall(geometry.indexCount() / 3); // Count triangles for indexed geometry
        } else {
            resourceManager->onDrawCall(geometry.vertexCount());
        }
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   QOpenGLBuffer& indexBuffer,
                                   QOpenGLBuffer& texCoordBuffer,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Bind PBR textures if available
    if (geometry.hasPBRTextures()) {

        // Bind base color texture to unit 0
        if (geometry.baseColorTextureId != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, geometry.baseColorTextureId);
        }

        // Bind metallic/roughness texture to unit 1
        if (geometry.metallicRoughnessTextureId != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, geometry.metallicRoughnessTextureId);
        }

        // Bind normal texture to unit 2
        if (geometry.normalTextureId != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, geometry.normalTextureId);
        }

        // Reset to texture unit 0
        glActiveTexture(GL_TEXTURE0);
    } else if (geometry.hasTexture()) {
        // Fallback to legacy single texture
        // Binding legacy texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, geometry.textureId);
    } else {
        // No textures available for rendering
    }

    // Upload vertex data (cached)
    vertexBuffer.bind();
    if (!geometry.verticesUploaded) {
        vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
        geometry.verticesUploaded = true;
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload color data (cached)
    colorBuffer.bind();
    if (!geometry.colorsUploaded) {
        colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
        geometry.colorsUploaded = true;
    }
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    // Upload normal data (cached)
    normalBuffer.bind();
    if (!geometry.normalsUploaded) {
        normalBuffer.allocate(geometry.normals.data(), geometry.normals.size() * sizeof(float));
        geometry.normalsUploaded = true;
    }
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);

    // Upload texture coordinate data if available (cached)
    if (!geometry.textureCoords.isEmpty()) {
        texCoordBuffer.bind();
        if (!geometry.texCoordsUploaded) {
            texCoordBuffer.allocate(geometry.textureCoords.data(), geometry.textureCoords.size() * sizeof(float));
            geometry.texCoordsUploaded = true;
        }
        glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(6);
    }

    // Draw the geometry - use indexed rendering if indices are available
    if (geometry.hasIndices()) {
        // Upload index data (cached)
        indexBuffer.bind();
        if (!geometry.indicesUploaded) {
            indexBuffer.allocate(geometry.indices.data(), geometry.indices.size() * sizeof(unsigned int));
            geometry.indicesUploaded = true;
        }

        // Draw using indices
        glDrawElements(drawMode, geometry.indexCount(), GL_UNSIGNED_INT, nullptr);
    } else {
        // Draw using vertex arrays (for lines and triangles)
        glDrawArrays(drawMode, 0, geometry.vertexCount());
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   QOpenGLBuffer& indexBuffer,
                                   QOpenGLBuffer& texCoordBuffer,
                                   ResourceManager* resourceManager,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    // Call the textured render method
    renderGeometry(geometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer, drawMode);

    // Track draw call statistics
    if (resourceManager) {
        if (geometry.hasIndices()) {
            resourceManager->onDrawCall(geometry.indexCount() / 3); // Count triangles for indexed geometry
        } else {
            resourceManager->onDrawCall(geometry.vertexCount());
        }
    }
}

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                          QOpenGLVertexArrayObject& vao,
                                          QOpenGLBuffer& vertexBuffer,
                                          QOpenGLBuffer& colorBuffer,
                                          QOpenGLBuffer& normalBuffer,
                                          QOpenGLBuffer& indexBuffer,
                                          QOpenGLBuffer& instanceBuffer,
                                          GLenum drawMode)
{
    if (!mInitialized || instances.isEmpty()) {
        return;
    }

    // Check if instancing functions are available
    if (!glVertexAttribDivisor || !glDrawElementsInstanced) {
        qWarning() << "GeometryManager: Instancing functions not available, falling back to individual cubes";
        // Fallback to individual cube rendering
        for (const auto& instance : instances) {
            GeometryData cubeGeometry = transformCubeTemplate(instance.transform, instance.color[0], instance.color[1], instance.color[2], instance.color[3]);
            renderGeometry(cubeGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode);
        }
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Upload cube template vertex and normal data
    vertexBuffer.bind();
    vertexBuffer.allocate(mCubeTemplate.vertices.data(), mCubeTemplate.vertices.size() * sizeof(float));
    // Pointer to vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Pointer to normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Upload cube template index data
    indexBuffer.bind();
    indexBuffer.allocate(mCubeTemplate.indices.data(), mCubeTemplate.indices.size() * sizeof(unsigned int));

    // Upload instance data to GPU
    instanceBuffer.bind();
    instanceBuffer.allocate(instances.data(), instances.size() * sizeof(CubeInstanceData));

    // Set up instance attributes
    // Color: location 3
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(0));
    glVertexAttribDivisor(3, 1);

    // Transform matrix: location 4-7
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(8 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(12 * sizeof(float)));
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(16 * sizeof(float)));
    glVertexAttribDivisor(7, 1);

    // Draw all instances with a single call
    glDrawElementsInstanced(drawMode, mCubeTemplate.indexCount(), GL_UNSIGNED_INT, nullptr, instances.size());

    // Clean up instance attributes
    glVertexAttribDivisor(3, 0);
    glVertexAttribDivisor(4, 0);
    glVertexAttribDivisor(5, 0);
    glVertexAttribDivisor(6, 0);
    glVertexAttribDivisor(7, 0);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);
    glDisableVertexAttribArray(7);
}

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                          QOpenGLVertexArrayObject& vao,
                                          QOpenGLBuffer& vertexBuffer,
                                          QOpenGLBuffer& colorBuffer,
                                          QOpenGLBuffer& normalBuffer,
                                          QOpenGLBuffer& indexBuffer,
                                          QOpenGLBuffer& instanceBuffer,
                                          ResourceManager* resourceManager,
                                          GLenum drawMode)
{
    if (instances.isEmpty()) {
        return;
    }

    // Call the original instanced render method
    renderInstancedCubes(instances, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, instanceBuffer, drawMode);

    // Track draw call statistics - one draw call for all instances
    if (resourceManager) {
        resourceManager->onDrawCall(instances.size() * (mCubeTemplate.indexCount() / 3)); // Count triangles for all instances
    }
}
