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

class QVector2D;

class CameraController
{
public:
    // One unit of distance shows about 5.8 rooms either side of the target vertically
    static constexpr float scmMinDistance = 0.05f;
    static constexpr float scmMaxDistance = 100.0f;
    // Degrees from straight down; the limits keep the camera above the map's plane
    static constexpr float scmMinTilt = 0.0f;
    static constexpr float scmMaxTilt = 82.0f;
    static constexpr float scmDefaultTilt = 41.0f;
    // The azimuth that puts the camera south of its target, so north is up the screen and east is to its right
    static constexpr float scmNorthUpAzimuth = 270.0f;
    // Vertical, in degrees
    static constexpr float scmFieldOfView = 60.0f;

    CameraController();
    ~CameraController();

    // Camera position and orientation
    void setPosition(float r, float theta, float phi);
    void setOrientation(float theta, float phi, float roll);
    void setScale(float scale);
    // Positive steps zoom in
    void zoomBy(float steps);
    // The zoom steps for a wheel event, matching the 2D map: positive means zoom in
    static float wheelZoomSteps(int angleDeltaY, bool fast, bool inverted);
    // The distance that shows this many rooms across the shorter side of the view at the target
    static float distanceToShow(float rooms, float aspectRatio);
    void setViewportSize(int width, int height);
    void shiftPerspective(float verticalAngle, float horizontalAngle, float rotationAngle);

    // Camera target control
    void setTarget(float x, float y, float z);
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
    QVector3D getPosition() const;
    float getRoll() const;
    QVector3D getTarget() const { return mTarget; }
    QVector3D getRightVector() const { return mRightVector; }
    QVector3D getUpVector() const { return mUpVector; }
    QVector3D screenRight() const;
    QVector3D screenUp() const { return mUpVector; }
    float getScale() const { return mDistance; }

    // View center (for external API compatibility)
    void setViewCenter(float x, float y, float z);

private:
    static QVector3D rotateAround(QVector3D currentVector, QVector3D rotationAxis, float rotationAngle);
    static QVector3D unrolledUp(float theta, float phi);
    QVector2D groundForward() const;
    // Camera parameters
    float mDistance = 1.0f;
    QVector3D mTarget;
    // NOTE: all these three vectors are and must remain normalized, and mRightVector stays
    // mPositionVector x mUpVector, which points to the screen's left
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
