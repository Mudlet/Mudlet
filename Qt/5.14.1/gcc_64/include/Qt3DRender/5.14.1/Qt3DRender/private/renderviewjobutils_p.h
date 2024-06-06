/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT3DRENDER_RENDERVIEWJOBUTILS_P_H
#define QT3DRENDER_RENDERVIEWJOBUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/qt3drender_global.h>
#include <Qt3DCore/qnodeid.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QMatrix4x4>
#include <Qt3DRender/private/uniform_p.h>
#include <Qt3DRender/private/handle_types_p.h>
#include <Qt3DRender/private/aligned_malloc_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {
class QFrameAllocator;
}

namespace Qt3DRender {
namespace Render {

class FrameGraphNode;
class ParameterManager;
class Effect;
class Entity;
class Material;
class RenderPass;
class RenderStateSet;
class Technique;
class RenderView;
class TechniqueFilter;
class RenderPassFilter;
class Renderer;
class NodeManagers;
class ShaderDataManager;
struct ShaderUniform;
class ShaderData;
class TextureManager;
class RenderStateManager;
class RenderStateCollection;

Q_AUTOTEST_EXPORT void setRenderViewConfigFromFrameGraphLeafNode(RenderView *rv,
                                                                 const FrameGraphNode *fgLeaf);

Q_AUTOTEST_EXPORT Technique *findTechniqueForEffect(NodeManagers *manager,
                                                    const TechniqueFilter *techniqueFilter,
                                                    Effect *effect);

typedef QVarLengthArray<RenderPass*, 4> RenderPassList;
Q_AUTOTEST_EXPORT RenderPassList findRenderPassesForTechnique(NodeManagers *manager,
                                                              const RenderPassFilter *passFilter,
                                                              Technique *technique);

// Extracts the type T from a QVariant v without using QVariant::value which is slow
// Note: Assumes you are 100% sure about the type you requested
template<typename T>
inline T variant_value(const QVariant &v)
{
    return *reinterpret_cast<const T *>(v.data());
}

struct ParameterInfo
{
    explicit ParameterInfo(const int nameId = -1, const HParameter &handle = HParameter());

    int nameId;
    HParameter handle;

    bool operator<(const int otherNameId) const Q_DECL_NOEXCEPT;
    bool operator<(const ParameterInfo &other) const Q_DECL_NOEXCEPT;
};
typedef QVector<ParameterInfo> ParameterInfoList;

struct RenderPassParameterData
{
    RenderPass *pass;
    ParameterInfoList parameterInfo;
};
QT3D_DECLARE_TYPEINFO_2(Qt3DRender, Render, RenderPassParameterData, Q_MOVABLE_TYPE)

using MaterialParameterGathererData = QHash<Qt3DCore::QNodeId, QVector<RenderPassParameterData>>;

Q_AUTOTEST_EXPORT void parametersFromMaterialEffectTechnique(ParameterInfoList *infoList,
                                                             ParameterManager *manager,
                                                             Material *material,
                                                             Effect *effect,
                                                             Technique *technique);

Q_AUTOTEST_EXPORT void addParametersForIds(ParameterInfoList *params, ParameterManager *manager,
                                           const QVector<Qt3DCore::QNodeId> &parameterIds);

template<class T>
void parametersFromParametersProvider(ParameterInfoList *infoList,
                                      ParameterManager *manager,
                                      T *provider)
{
    addParametersForIds(infoList, manager, provider->parameters());
}

Q_AUTOTEST_EXPORT ParameterInfoList::const_iterator findParamInfo(ParameterInfoList *infoList,
                                                                  const int nameId);

Q_AUTOTEST_EXPORT void addStatesToRenderStateSet(RenderStateSet *stateSet,
                                                 const QVector<Qt3DCore::QNodeId> stateIds,
                                                 RenderStateManager *manager);

typedef QHash<int, QVariant> UniformBlockValueBuilderHash;

struct Q_AUTOTEST_EXPORT UniformBlockValueBuilder
{
    UniformBlockValueBuilder();
    ~UniformBlockValueBuilder();

    QT3D_ALIGNED_MALLOC_AND_FREE()

    void buildActiveUniformNameValueMapHelper(ShaderData *currentShaderData,
                                              const QString &blockName,
                                              const QString &qmlPropertyName,
                                              const QVariant &value);
    void buildActiveUniformNameValueMapStructHelper(ShaderData *rShaderData,
                                                    const QString &blockName,
                                                    const QString &qmlPropertyName = QString());

    bool updatedPropertiesOnly;
    QHash<QString, ShaderUniform> uniforms;
    UniformBlockValueBuilderHash activeUniformNamesToValue;
    ShaderDataManager *shaderDataManager;
    TextureManager *textureManager;
    Matrix4x4 viewMatrix;
};

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDERVIEWJOBUTILS_P_H
