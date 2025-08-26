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

#include "ShaderManager.h"
#include "ResourceManager.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <chrono>
#include "post_guard.h"

#include "utils.h"

ShaderManager::ShaderManager(ResourceManager* resourceManager, QObject* parent)
    : QObject(parent)
    , mFileWatcher(nullptr)
    , mReloadTimer(nullptr)
    , mDevelopmentMode(false)
    , mInitialized(false)
    , mUniformMVP(-1)
    , mUniformModel(-1)
    , mUniformNormalMatrix(-1)
    , mResourceManager(resourceManager)
{
    mReloadTimer = new QTimer(this);
    mReloadTimer->setSingleShot(true);
    mReloadTimer->setInterval(std::chrono::milliseconds(50));
    connect(mReloadTimer, &QTimer::timeout, this, &ShaderManager::delayedReload);
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

bool ShaderManager::initialize()
{
    if (mInitialized) {
        return true;
    }

    initializeOpenGLFunctions();
    
    mDevelopmentMode = detectDevelopmentMode();
    qDebug() << "ShaderManager: Hot-reload" << (mDevelopmentMode ? "enabled" : "disabled (using embedded shaders)");
    
    if (mDevelopmentMode) {
        // Try multiple well-known locations for shader files
        QStringList possiblePaths = {
            // CMake build from build/src/
            qsl("../src/shaders/"),
            // CMake build from build/
            qsl("src/shaders/"),
            // In-source build
            qsl("../../src/shaders/"),
            // Qt Creator build
            qsl("../../../src/shaders/"),
            // Relative to current directory
            qsl("shaders/")
        };
        
        QString appDir = QCoreApplication::applicationDirPath();
        QString foundPath;
        
        for (const QString& relativePath : possiblePaths) {
            QString testPath = QDir(appDir).filePath(relativePath);
            QDir testDir(testPath);
            if (testDir.exists() && 
                QFile::exists(testDir.filePath(qsl("vertex.glsl"))) && 
                QFile::exists(testDir.filePath(qsl("fragment.glsl")))) {
                foundPath = testPath;
                break;
            }
        }
        
        if (foundPath.isEmpty()) {
            qWarning() << "ShaderManager: Could not find shader directory, trying fallback locations";
            mVertexShaderPath = QDir(appDir).canonicalPath() + qsl("/../src/shaders/vertex.glsl");
            mFragmentShaderPath = QDir(appDir).canonicalPath() + qsl("/../src/shaders/fragment.glsl");
        } else {
            mVertexShaderPath = QDir(foundPath).canonicalPath() + qsl("/vertex.glsl");
            mFragmentShaderPath = QDir(foundPath).canonicalPath() + qsl("/fragment.glsl");
        }
        
        mFileWatcher = new QFileSystemWatcher(this);
        if (QFile::exists(mVertexShaderPath)) {
            mFileWatcher->addPath(mVertexShaderPath);
        }
        if (QFile::exists(mFragmentShaderPath)) {
            mFileWatcher->addPath(mFragmentShaderPath);
        }
        connect(mFileWatcher, &QFileSystemWatcher::fileChanged, this, &ShaderManager::onShaderFileChanged);
        
        qDebug() << "ShaderManager: Watching shader files:";
        qDebug() << "  Vertex:" << mVertexShaderPath << "(exists:" << QFile::exists(mVertexShaderPath) << ")";
        qDebug() << "  Fragment:" << mFragmentShaderPath << "(exists:" << QFile::exists(mFragmentShaderPath) << ")";
    }
    
    mInitialized = createShaderProgram();
    return mInitialized;
}

QOpenGLShaderProgram* ShaderManager::getMainShaderProgram()
{
    if (!mInitialized || !mShaderProgram) {
        qWarning() << "ShaderManager: Shader program not initialized";
        return nullptr;
    }
    return mShaderProgram.get();
}

bool ShaderManager::reloadShaders()
{
    qDebug() << "ShaderManager: Reloading shaders...";
    
    mShaderProgram.reset();
    mUniformMVP = -1;
    mUniformModel = -1;
    mUniformNormalMatrix = -1;
    
    bool success = createShaderProgram();
    if (success) {
        qDebug() << "ShaderManager: Shaders reloaded successfully";
        emit shadersReloaded();
    } else {
        qWarning() << "ShaderManager: Failed to reload shaders";
    }
    
    return success;
}

void ShaderManager::cleanup()
{
    mShaderProgram.reset();
    mInitialized = false;
}

void ShaderManager::onShaderFileChanged(const QString& path)
{
    qDebug() << "ShaderManager: Shader file changed:" << path;
    
    if (mFileWatcher) {
        mFileWatcher->addPath(path);
    }
    
    mReloadTimer->start();
}

void ShaderManager::delayedReload()
{
    reloadShaders();
}

QString ShaderManager::loadShaderSource(const QString& shaderName)
{
    QString source;
    
    if (mDevelopmentMode) {
        QString filePath = (shaderName == qsl("vertex")) ? mVertexShaderPath : mFragmentShaderPath;
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            source = file.readAll();
            qDebug() << "ShaderManager: Loaded" << shaderName << "shader from file:" << filePath;
        } else {
            qWarning() << "ShaderManager: Failed to load" << shaderName << "shader from file:" << filePath;
        }
    } else {
        QString resourcePath = qsl(":/shaders/%1.glsl").arg(shaderName);
        QFile file(resourcePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            source = file.readAll();
            qDebug() << "ShaderManager: Loaded" << shaderName << "shader from resource:" << resourcePath;
        } else {
            qWarning() << "ShaderManager: Failed to load" << shaderName << "shader from resource:" << resourcePath;
        }
    }
    
    
    return source;
}

bool ShaderManager::createShaderProgram()
{
    mShaderProgram = std::make_unique<QOpenGLShaderProgram>();

    QString vertexSource = loadShaderSource(qsl("vertex"));
    QString fragmentSource = loadShaderSource(qsl("fragment"));
    
    if (vertexSource.isEmpty() || fragmentSource.isEmpty()) {
        qWarning() << "ShaderManager: Empty shader sources";
        return false;
    }

    if (!mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        qWarning() << "ShaderManager: Failed to compile vertex shader:" << mShaderProgram->log();
        return false;
    }
    
    if (mResourceManager) {
        mResourceManager->onShaderCreated();
    }

    if (!mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        qWarning() << "ShaderManager: Failed to compile fragment shader:" << mShaderProgram->log();
        return false;
    }
    
    if (mResourceManager) {
        mResourceManager->onShaderCreated();
    }

    if (!mShaderProgram->link()) {
        qWarning() << "ShaderManager: Failed to link shader program:" << mShaderProgram->log();
        return false;
    }

    mUniformMVP = mShaderProgram->uniformLocation(qsl("uMVP"));
    mUniformModel = mShaderProgram->uniformLocation(qsl("uModel"));
    mUniformNormalMatrix = mShaderProgram->uniformLocation(qsl("uNormalMatrix"));

    if (mUniformMVP == -1) {
        qWarning() << "ShaderManager: Failed to get MVP uniform location";
        return false;
    }

    qDebug() << "ShaderManager: Shaders compiled successfully. Uniform locations - MVP:" << mUniformMVP << "Model:" << mUniformModel << "Normal:" << mUniformNormalMatrix;
    return true;
}

bool ShaderManager::detectDevelopmentMode()
{
#ifdef MUDLET_SHADER_HOT_RELOAD
    return true;
#else
    return false;
#endif
}


