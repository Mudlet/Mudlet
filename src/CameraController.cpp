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

#include <QVector2D>
#include <QtMath>

CameraController::CameraController()
{
    mTarget = QVector3D(0.0f, 0.0f, 0.0f);
    setDefaultView();
    calculateModelMatrix();
}

CameraController::~CameraController() = default;

void CameraController::setPosition(float r, float theta, float phi)
{
    theta = qBound(scmMinTilt, theta, scmMaxTilt);
    mDistance = qBound(scmMinDistance, r, scmMaxDistance);
    const float thetaRadians = qDegreesToRadians(theta);
    const float phiRadians = qDegreesToRadians(phi);
    mPositionVector = QVector3D(std::sin(thetaRadians) * std::cos(phiRadians), std::sin(thetaRadians) * std::sin(phiRadians), std::cos(thetaRadians));
    mUpVector = unrolledUp(theta, phi);
    mRightVector = QVector3D::crossProduct(mPositionVector, mUpVector);
}

void CameraController::setOrientation(float theta, float phi, float roll)
{
    setPosition(mDistance, theta, phi);
    if (roll != 0.0f) {
        mUpVector = rotateAround(mUpVector, mPositionVector, roll).normalized();
        mRightVector = QVector3D::crossProduct(mPositionVector, mUpVector).normalized();
    }
}

QVector3D CameraController::unrolledUp(float theta, float phi)
{
    const float thetaRadians = qDegreesToRadians(theta);
    const float phiRadians = qDegreesToRadians(phi);
    return QVector3D(-std::cos(thetaRadians) * std::cos(phiRadians), -std::cos(thetaRadians) * std::sin(phiRadians), std::sin(thetaRadians));
}

void CameraController::setTarget(float x, float y, float z)
{
    mTarget.setX(x);
    mTarget.setY(y);
    mTarget.setZ(z);
};

void CameraController::translateTargetUp()
{
    mTarget.setZ(mTarget.z() + 1);
}
void CameraController::translateTargetDown()
{
    mTarget.setZ(mTarget.z() - 1);
}
void CameraController::translateTargetLeft()
{
    const QVector2D direction = mRightVector.toVector2D();
    if (direction.length() < 1e-4f) {
        return;
    }
    mTarget -= QVector3D(direction.normalized() * 0.1f * mDistance, 0.0f);
}
void CameraController::translateTargetRight()
{
    const QVector2D direction = mRightVector.toVector2D();
    if (direction.length() < 1e-4f) {
        return;
    }
    mTarget += QVector3D(direction.normalized() * 0.1f * mDistance, 0.0f);
}
void CameraController::translateTargetForward()
{
    mTarget += QVector3D(groundForward() * 0.1f * mDistance, 0.0f);
}
void CameraController::translateTargetBackward()
{
    mTarget -= QVector3D(groundForward() * 0.1f * mDistance, 0.0f);
}

QVector2D CameraController::groundForward() const
{
    // Looking straight down there is no horizontal offset to the camera, but the screen still has an up
    QVector2D direction = mUpVector.toVector2D();
    if (direction.length() < 1e-4f) {
        direction = -mPositionVector.toVector2D();
    }
    return direction.normalized();
}

void CameraController::snapTargetToGrid()
{
    setTarget(std::round(mTarget.x()), std::round(mTarget.y()), std::round(mTarget.z()));
}

void CameraController::setScale(float scale)
{
    mDistance = qBound(scmMinDistance, scale, scmMaxDistance);
}

void CameraController::zoomBy(float steps)
{
    setScale(mDistance * std::pow(1.1f, -steps));
}

int CameraController::wheelZoomSteps(int angleDeltaY, bool fast, bool inverted)
{
    // One notch of a mouse wheel is 120, and the 2D map steps five times as fast with Ctrl
    const int steps = qRound(angleDeltaY * (fast ? 5.0 : 1.0) / 120.0);
    return inverted ? -steps : steps;
}

float CameraController::distanceToShow(float rooms, float aspectRatio)
{
    const float shorterSide = aspectRatio > 0.0f ? qMin(1.0f, aspectRatio) : 1.0f;
    // A unit of distance is ten rooms (see calculateViewMatrix)
    const float halfHeightPerUnit = 10.0f * std::tan(qDegreesToRadians(scmFieldOfView / 2.0f));
    return qBound(scmMinDistance, rooms / 2.0f / (halfHeightPerUnit * shorterSide), scmMaxDistance);
}

void CameraController::setViewportSize(int width, int height)
{
    mViewportWidth = width;
    mViewportHeight = height;
}

void CameraController::shiftPerspective(float verticalAngle, float horizontalAngle, float rotationAngle)
{
    if (verticalAngle != 0) {
        mPositionVector = rotateAround(mPositionVector, mRightVector, verticalAngle);
        mUpVector = QVector3D::normal(mRightVector, mPositionVector);
    }
    if (horizontalAngle != 0) {
        mPositionVector = rotateAround(mPositionVector, mUpVector, horizontalAngle);
        mRightVector = QVector3D::normal(mPositionVector, mUpVector);
    }
    if (rotationAngle != 0) {
        mUpVector = rotateAround(mUpVector, mPositionVector, rotationAngle);
        mUpVector /= mUpVector.length();
        mRightVector = QVector3D::normal(mPositionVector, mUpVector);
    }
}

QVector3D CameraController::rotateAround(QVector3D currentVector, QVector3D rotationAxis, float rotationAngle)
{
    rotationAngle = qDegreesToRadians(rotationAngle);
    // Apply Rodrigues rotation formula
    return std::cos(rotationAngle) * currentVector + std::sin(rotationAngle) * QVector3D::crossProduct(rotationAxis, currentVector)
           + QVector3D::dotProduct(rotationAxis, currentVector) * (1 - std::cos(rotationAngle)) * rotationAxis;
}

QVector3D CameraController::getPosition() const
{
    const float theta = qRadiansToDegrees(std::acos(qBound(-1.0f, mPositionVector.z(), 1.0f)));
    if (mPositionVector.toVector2D().length() < 1e-6f) {
        // Looking straight down, the azimuth is wherever the top of the screen faces
        return QVector3D(mDistance, theta, qRadiansToDegrees(std::atan2(-mUpVector.y(), -mUpVector.x())));
    }
    return QVector3D(mDistance, theta, qRadiansToDegrees(std::atan2(mPositionVector.y(), mPositionVector.x())));
}

float CameraController::getRoll() const
{
    const QVector3D position = getPosition();
    const QVector3D unrolled = unrolledUp(position.y(), position.z());
    const float sine = QVector3D::dotProduct(mUpVector, QVector3D::crossProduct(mPositionVector, unrolled));
    const float cosine = QVector3D::dotProduct(mUpVector, unrolled);
    return qRadiansToDegrees(std::atan2(sine, cosine));
}

QVector3D CameraController::screenRight() const
{
    return QVector3D::crossProduct(mUpVector, mPositionVector).normalized();
}

void CameraController::setDefaultView()
{
    setPosition(mDistance, scmDefaultTilt, scmNorthUpAzimuth);
}

void CameraController::setSideView()
{
    setPosition(mDistance, scmMaxTilt, scmNorthUpAzimuth);
}

void CameraController::setTopView()
{
    mPositionVector = QVector3D(0.0f, 0.0f, 1.0f);
    mUpVector = QVector3D(0.0f, 1.0f, 0.0f);
    mRightVector = QVector3D::crossProduct(mPositionVector, mUpVector);
}

void CameraController::setGridMode(bool enabled)
{
    mGridMode = enabled;
    if (enabled) {
        setTopView();
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
    setTarget(x, y, z);
}

void CameraController::calculateProjectionMatrix()
{
    // Set up projection matrix with fixed FOV
    mProjectionMatrix.setToIdentity();
    const float aspectRatio = static_cast<float>(mViewportWidth) / static_cast<float>(mViewportHeight);
    // Keep the field of view constant, adjust camera distance with scale instead
    mProjectionMatrix.perspective(scmFieldOfView, aspectRatio, 0.0001f, 10000.0f);
}

void CameraController::calculateViewMatrix()
{
    // Set up view matrix (camera)
    mViewMatrix.setToIdentity();

    // We shrink the coordinate system by a factor of 10, why? No idea
    const QVector3D target = mTarget / 10;

    // Original uses xRot, yRot, zRot as camera position offsets, not rotation angles
    // gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);

    // Create view matrix to look at target from camera position
    mViewMatrix.lookAt(target + (mPositionVector * mDistance), target, mUpVector);

    // Scale the world to match original rendering
    mViewMatrix.scale(0.1f, 0.1f, 0.1f);
}

void CameraController::calculateModelMatrix()
{
    // Model matrix will be set per object during rendering
    mModelMatrix.setToIdentity();
}
