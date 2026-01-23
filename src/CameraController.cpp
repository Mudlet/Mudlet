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

#include <QtGlobal>

using namespace std::numbers;

CameraController::CameraController()
{
    mTarget = QVector3D(0.0f, 0.0f, 0.0f);
    setDefaultView();
    calculateModelMatrix();
}

CameraController::~CameraController() = default;

void CameraController::setPosition(float r, float theta, float phi)
{
    // convert from degrees into radians
    theta = qBound(2.0f, theta, 82.0f);
    theta = theta/360 * 2 * pi;
    phi = phi/360 * 2 * pi;

    mDistance = r;
    mPositionVector.setX(std::sin(theta) * std::cos(phi));
    mPositionVector.setY(std::sin(theta) * std::sin(phi));
    mPositionVector.setZ(std::cos(theta));
    mUpVector.setX(std::sin(theta - pi/2) * std::cos(phi));
    mUpVector.setY(std::sin(theta - pi/2) * std::sin(phi));
    mUpVector.setZ(std::cos(theta - pi/2));
    mRightVector = QVector3D::crossProduct(mPositionVector, mUpVector);
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
    const float translationSpeed = 0.1f * mDistance;
    const float normFactor = std::sqrt(mRightVector.x() * mRightVector.x() + mRightVector.y() * mRightVector.y());
    mTarget.setX(mTarget.x() - translationSpeed * mRightVector.x() / normFactor);
    mTarget.setY(mTarget.y() - translationSpeed * mRightVector.y() / normFactor);
}
void CameraController::translateTargetRight()
{
    const float translationSpeed = 0.1f * mDistance;
    const float normFactor = std::sqrt(mRightVector.x() * mRightVector.x() + mRightVector.y() * mRightVector.y());
    mTarget.setX(mTarget.x() + translationSpeed * mRightVector.x() / normFactor);
    mTarget.setY(mTarget.y() + translationSpeed * mRightVector.y() / normFactor);
}
void CameraController::translateTargetForward()
{
    const float translationSpeed = 0.1f * mDistance;
    const float normFactor = std::sqrt(mPositionVector.x() * mPositionVector.x() + mPositionVector.y() * mPositionVector.y());
    mTarget.setX(mTarget.x() - translationSpeed * mPositionVector.x() / normFactor);
    mTarget.setY(mTarget.y() - translationSpeed * mPositionVector.y() / normFactor);
}
void CameraController::translateTargetBackward()
{
    const float translationSpeed = 0.1f * mDistance;
    const float normFactor = std::sqrt(mPositionVector.x() * mPositionVector.x() + mPositionVector.y() * mPositionVector.y());
    mTarget.setX(mTarget.x() + translationSpeed * mPositionVector.x() / normFactor);
    mTarget.setY(mTarget.y() + translationSpeed * mPositionVector.y() / normFactor);
}

void CameraController::snapTargetToGrid()
{
    setTarget(std::round(mTarget.x()), std::round(mTarget.y()), std::round(mTarget.z()));
}

void CameraController::setScale(float scale)
{
    // Clamp scale to reasonable bounds to prevent zoom issues
    mDistance = qBound(0.01f, scale, 100.0f);
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
    // convert degrees to radians
    rotationAngle = rotationAngle / 360 * 2 * pi;
    // Apply Rodrigues rotation formula
    return std::cos(rotationAngle) * currentVector
        + std::sin(rotationAngle) * QVector3D::crossProduct(rotationAxis, currentVector)
        + QVector3D::dotProduct(rotationAxis, currentVector) * (1 - std::cos(rotationAngle)) * rotationAxis;
}

QVector3D CameraController::getPosition()
{
    if (mPositionVector.x() == 0 && mPositionVector.y() == 0) {
        return QVector3D(mDistance, 0.0f, 0.0f);
    }

    const float toDegrees = 180.0f / pi;
    const float theta = toDegrees * std::acos(mPositionVector.z());
    const float phi = toDegrees * std::atan2(mPositionVector.y(), mPositionVector.x());
    return QVector3D(mDistance, theta, phi);
}

void CameraController::setDefaultView()
{
    // default camera position 30 degrees from directly above, and 15 degrees rotated to the left (from forward == north)
    setPosition(1.0f, 60.0f, static_cast<float>(270 - 15));
}

void CameraController::setSideView()
{
    mPositionVector = QVector3D(1.0f, 0.0f, 0.0f);
    mRightVector = QVector3D(0.0f, 1.0f, 0.0f);
    mUpVector = QVector3D(0.0f, 0.0f, 1.0f);
}

void CameraController::setTopView()
{
    mPositionVector = QVector3D(0.0f, 0.0f, 1.0f);
    mRightVector = QVector3D(0.0f, 1.0f, 0.0f);
    mUpVector = QVector3D(1.0f, 0.0f, 0.0f);
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
    // Keep FOV constant at 60 degrees, adjust camera distance with scale instead
    mProjectionMatrix.perspective(60.0f, aspectRatio, 0.0001f, 10000.0f);
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
