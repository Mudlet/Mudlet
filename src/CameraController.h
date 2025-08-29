#ifndef MUDLET_CAMERA_CONTROLLER_H
#define MUDLET_CAMERA_CONTROLLER_H

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

#include "pre_guard.h"
#include <QMatrix4x4>
#include <QVector3D>
#include "post_guard.h"

class CameraController
{
public:
    CameraController();
    ~CameraController();

    // Camera position and orientation
    void setRotation(float xRot, float yRot, float zRot);
    void setPosition(float centerX, float centerY, float centerZ);
    void setScale(float scale);
    void setViewportSize(int width, int height);
    
    // View presets
    void setDefaultView();
    void setSideView();  
    void setTopView();
    void setGridMode(bool enabled);
    
    // Matrix generation
    void updateMatrices();
    QMatrix4x4 getProjectionMatrix() const { return mProjectionMatrix; }
    QMatrix4x4 getViewMatrix() const { return mViewMatrix; }
    QMatrix4x4 getModelMatrix() const { return mModelMatrix; }
    
    // Camera state
    float getXRot() const { return mXRot; }
    float getYRot() const { return mYRot; }
    float getZRot() const { return mZRot; }
    float getScale() const { return mScale; }
    float getCenterX() const { return mCenterX; }
    float getCenterY() const { return mCenterY; }
    float getCenterZ() const { return mCenterZ; }
    
    // View center (for external API compatibility)
    void setViewCenter(float x, float y, float z);

private:
    // Camera parameters
    float mXRot = 1.0f;
    float mYRot = 5.0f; 
    float mZRot = 10.0f;
    float mScale = 1.0f;
    
    float mCenterX = 0.0f;
    float mCenterY = 0.0f;
    float mCenterZ = 0.0f;
    
    int mViewportWidth = 400;
    int mViewportHeight = 400;
    
    bool mGridMode = false;
    
    // Transformation matrices
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
    
    // Internal matrix calculation
    void calculateProjectionMatrix();
    void calculateViewMatrix();
    void calculateModelMatrix();
};

#endif // MUDLET_CAMERA_CONTROLLER_H
