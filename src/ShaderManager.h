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

#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QFileSystemWatcher>
#include <QTimer>
#include <memory>

class ResourceManager;

class ShaderManager : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ShaderManager(ResourceManager* resourceManager = nullptr, QObject* parent = nullptr);
    ~ShaderManager();

    bool initialize();
    QOpenGLShaderProgram* getMainShaderProgram();
    bool reloadShaders();
    void cleanup();

    int getUniformMVP() const { return mUniformMVP; }
    int getUniformModel() const { return mUniformModel; }
    int getUniformNormalMatrix() const { return mUniformNormalMatrix; }

    bool isDevelopmentMode() const { return mDevelopmentMode; }

signals:
    void shadersReloaded();

private slots:
    void onShaderFileChanged(const QString& path);
    void delayedReload();

private:
    QString loadShaderSource(const QString& shaderName);
    bool createShaderProgram();
    bool detectDevelopmentMode();

    std::unique_ptr<QOpenGLShaderProgram> mShaderProgram;
    QFileSystemWatcher* mFileWatcher{nullptr};
    QTimer* mReloadTimer{nullptr};

    bool mDevelopmentMode;
    bool mInitialized;

    int mUniformMVP;
    int mUniformModel;
    int mUniformNormalMatrix;

    QString mVertexShaderPath;
    QString mFragmentShaderPath;

    ResourceManager* mResourceManager;
};

#endif // SHADERMANAGER_H
