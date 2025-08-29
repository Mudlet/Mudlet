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

#include "CameraController.h"

#include "pre_guard.h"
#include <QtGlobal>
#include "post_guard.h"

CameraController::CameraController()
{
    // Initialize with default view parameters (matching original)
    mXRot = 1.0f;
    mYRot = 5.0f; 
    mZRot = 10.0f;
    mScale = 1.0f;
    
    calculateModelMatrix();
}

CameraController::~CameraController() = default;

void CameraController::setRotation(float xRot, float yRot, float zRot)
{
    mXRot = xRot;
    mYRot = yRot;
    mZRot = zRot;
}

void CameraController::setPosition(float centerX, float centerY, float centerZ)
{
    mCenterX = centerX;
    mCenterY = centerY;
    mCenterZ = centerZ;
}

void CameraController::setScale(float scale)
{
    // Clamp scale to reasonable bounds to prevent zoom issues
    mScale = qBound(0.01f, scale, 100.0f);
}

void CameraController::setViewportSize(int width, int height)
{
    mViewportWidth = width;
    mViewportHeight = height;
}

void CameraController::setDefaultView()
{
    mXRot = 1.0f;
    mYRot = 5.0f;
    mZRot = 10.0f;
    mScale = 1.0f;
}

void CameraController::setSideView()
{
    mXRot = 7.0f;
    mYRot = -10.0f;
    mZRot = 0.0f;
    mScale = 1.0f;
}

void CameraController::setTopView()
{
    mXRot = 0.0f;
    mYRot = 0.0f;
    mZRot = 15.0f;
    mScale = 1.0f;
}

void CameraController::setGridMode(bool enabled)
{
    mGridMode = enabled;
    if (enabled) {
        // Grid mode uses top view camera position
        mXRot = 0.0f;
        mYRot = 0.0f;
        mZRot = 15.0f;
    }
}

void CameraController::updateMatrices()
{
    calculateProjectionMatrix();
    calculateViewMatrix();
    calculateModelMatrix();
}

void CameraController::setViewCenter(float x, float y, float z)
{
    setPosition(x, y, z);
}

void CameraController::calculateProjectionMatrix()
{
    // Set up projection matrix with fixed FOV
    mProjectionMatrix.setToIdentity();
    const float aspectRatio = static_cast<float>(mViewportWidth) / static_cast<float>(mViewportHeight);
    // Keep FOV constant at 60 degrees, adjust camera distance with scale instead
    mProjectionMatrix.perspective(60.0f, aspectRatio, 0.0001f, 10000.0f);
}

void CameraController::calculateViewMatrix()
{
    // Set up view matrix (camera)
    mViewMatrix.setToIdentity();

    // Original uses xRot, yRot, zRot as camera position offsets, not rotation angles
    // gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);

    // Calculate camera position with offsets (scale appropriately for our coordinate system)
    const float px = mCenterX * 0.1f;
    const float py = mCenterY * 0.1f;
    const float pz = mCenterZ * 0.1f;

    // Camera position with offsets, scaled by mScale for zoom
    // The original offsets are scaled by the zoom factor to maintain proper distance
    const float scaleMultiplier = 1.0f / mScale;
    const float cameraX = px + (mXRot * scaleMultiplier);
    const float cameraY = py + (mYRot * scaleMultiplier);
    const float cameraZ = pz + (mZRot * scaleMultiplier);

    // Target position (map center)
    const float targetX = px;
    const float targetY = py;
    const float targetZ = pz;

    // Create view matrix to look at target from camera position
    mViewMatrix.lookAt(QVector3D(cameraX, cameraY, cameraZ), QVector3D(targetX, targetY, targetZ), QVector3D(0.0f, 1.0f, 0.0f));

    // Scale the world to match original rendering
    mViewMatrix.scale(0.1f, 0.1f, 0.1f);
}

void CameraController::calculateModelMatrix()
{
    // Model matrix will be set per object during rendering
    mModelMatrix.setToIdentity();
}
