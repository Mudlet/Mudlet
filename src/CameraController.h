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

#include <QMatrix4x4>
#include <QVector3D>
#include <numbers>
//#include <boost/math/constants/constants.hpp>

class CameraController
{
public:
    CameraController();
    ~CameraController();

    // Camera position and orientation
    void setPosition(float r, float theta, float phi);
    void setScale(float scale);
    void setViewportSize(int width, int height);
    void shiftPerspective(float verticalAngle, float horizontalAngle, float rotationAngle);

    // Camera target control
    void setTarget(float x, float y, float z);
    void translateTargetUp();
    void translateTargetDown();
    void translateTargetLeft();
    void translateTargetRight();
    void translateTargetForward();
    void translateTargetBackward();
    void snapTargetToGrid();

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
    QVector3D getPosition();
    QVector3D getTarget() const { return mTarget; }
    // QVector3D getUp() const { return mUpVector; }
    float getScale() const { return mDistance; }

    // View center (for external API compatibility)
    void setViewCenter(float x, float y, float z);

private:
    QVector3D rotateAround(QVector3D currentVector, QVector3D rotationAxis, float rotationAngle);
    // Camera parameters
    float mDistance;
    QVector3D mTarget;
    // NOTE: all these three vectors are and must remain normalized
    QVector3D mPositionVector;
    QVector3D mRightVector;
    QVector3D mUpVector;

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
