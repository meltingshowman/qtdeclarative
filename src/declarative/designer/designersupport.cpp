/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "designersupport.h"
#include "qsgitem_p.h"

#include <QtDeclarative/private/qsgshadereffectsource_p.h>
#include <QtDeclarative/private/qsgrectangle_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>
#include <QtDeclarative/private/qsgview_p.h>
#include <QtDeclarative/private/qdeclarativestategroup_p.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

DesignerSupport::DesignerSupport()
{
}

DesignerSupport::~DesignerSupport()
{
    QHash<QSGItem*, QSGShaderEffectTexture*>::iterator iterator;

    for (iterator = m_itemTextureHash.begin(); iterator != m_itemTextureHash.end(); ++iterator) {
        QSGShaderEffectTexture *texture = iterator.value();
        QSGItem *item = iterator.key();
        QSGItemPrivate::get(item)->derefFromEffectItem(true);
        delete texture;
    }
}

void DesignerSupport::refFromEffectItem(QSGItem *referencedItem, bool hide)
{
    if (referencedItem == 0)
        return;

    QSGItemPrivate::get(referencedItem)->refFromEffectItem(hide);
    QSGCanvasPrivate::get(referencedItem->canvas())->updateDirtyNode(referencedItem);

    Q_ASSERT(QSGItemPrivate::get(referencedItem)->rootNode);

    if (!m_itemTextureHash.contains(referencedItem)) {
        QSGShaderEffectTexture *texture = new QSGShaderEffectTexture(referencedItem);

        texture->setLive(true);
        texture->setItem(QSGItemPrivate::get(referencedItem)->rootNode);
        texture->setRect(referencedItem->boundingRect());
        texture->setSize(referencedItem->boundingRect().size().toSize());
        texture->setRecursive(true);
        texture->setFormat(GL_RGBA8);
        texture->setHasMipmaps(false);

        m_itemTextureHash.insert(referencedItem, texture);
    }
}

void DesignerSupport::derefFromEffectItem(QSGItem *referencedItem, bool unhide)
{
    if (referencedItem == 0)
        return;

    delete m_itemTextureHash.take(referencedItem);
    QSGItemPrivate::get(referencedItem)->derefFromEffectItem(unhide);
}

QImage DesignerSupport::renderImageForItem(QSGItem *referencedItem)
{
    if (referencedItem == 0 || referencedItem->parentItem() == 0) {
        qDebug() << __FILE__ << __LINE__ << "Warning: Item can be rendered.";
        return QImage();
    }

    QSGShaderEffectTexture *renderTexture = m_itemTextureHash.value(referencedItem);

    Q_ASSERT(renderTexture);
    if (renderTexture == 0)
         return QImage();
    renderTexture->setRect(referencedItem->boundingRect());
    renderTexture->setSize(referencedItem->boundingRect().size().toSize());
    renderTexture->updateTexture();

    QImage renderImage = renderTexture->toImage();
    renderImage = renderImage.mirrored(false, true);

    if (renderImage.size().isEmpty())
        qDebug() << __FILE__ << __LINE__ << "Warning: Image is empty.";

    qDebug() << __FUNCTION__ << renderImage.size();

    return renderImage;
}

bool DesignerSupport::isDirty(QSGItem *referencedItem, DirtyType dirtyType)
{
    if (referencedItem == 0)
        return false;

    return QSGItemPrivate::get(referencedItem)->dirtyAttributes & dirtyType;
}

void DesignerSupport::resetDirty(QSGItem *referencedItem)
{
    if (referencedItem == 0)
        return;

    QSGItemPrivate::get(referencedItem)->dirtyAttributes = 0x0;
    QSGItemPrivate::get(referencedItem)->removeFromDirtyList();
}

QTransform DesignerSupport::canvasTransform(QSGItem *referencedItem)
{
    if (referencedItem == 0)
        return QTransform();

    return QSGItemPrivate::get(referencedItem)->itemToCanvasTransform();
}

QTransform DesignerSupport::parentTransform(QSGItem *referencedItem)
{
    if (referencedItem == 0)
        return QTransform();

    QTransform parentTransform;

    QSGItemPrivate::get(referencedItem)->itemToParentTransform(parentTransform);

    return parentTransform;
}

QString propertyNameForAnchorLine(const QSGAnchorLine::AnchorLine &anchorLine)
{
    switch (anchorLine) {
        case QSGAnchorLine::Left: return QLatin1String("left");
        case QSGAnchorLine::Right: return QLatin1String("right");
        case QSGAnchorLine::Top: return QLatin1String("top");
        case QSGAnchorLine::Bottom: return QLatin1String("bottom");
        case QSGAnchorLine::HCenter: return QLatin1String("horizontalCenter");
        case QSGAnchorLine::VCenter: return QLatin1String("verticalCenter");
        case QSGAnchorLine::Baseline: return QLatin1String("baseline");
        case QSGAnchorLine::Invalid:
        default: return QString();
    }
}

bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << QLatin1String("anchors.top")
                                                    << QLatin1String("anchors.left")
                                                    << QLatin1String("anchors.right")
                                                    << QLatin1String("anchors.bottom")
                                                    << QLatin1String("anchors.verticalCenter")
                                                    << QLatin1String("anchors.horizontalCenter")
                                                    << QLatin1String("anchors.fill")
                                                    << QLatin1String("anchors.centerIn")
                                                    << QLatin1String("anchors.baseline"));

    return anchorNameList.contains(name);
}

bool DesignerSupport::isAnchoredTo(QSGItem *fromItem, QSGItem *toItem)
{
    Q_ASSERT(dynamic_cast<QSGItemPrivate*>(QSGItemPrivate::get(fromItem)));
    QSGItemPrivate *fromItemPrivate = static_cast<QSGItemPrivate*>(QSGItemPrivate::get(fromItem));
    QSGAnchors *anchors = fromItemPrivate->anchors();
    return anchors->fill() == toItem
            || anchors->centerIn() == toItem
            || anchors->bottom().item == toItem
            || anchors->top().item == toItem
            || anchors->left().item == toItem
            || anchors->right().item == toItem
            || anchors->verticalCenter().item == toItem
            || anchors->horizontalCenter().item == toItem
            || anchors->baseline().item == toItem;
}

bool DesignerSupport::areChildrenAnchoredTo(QSGItem *fromItem, QSGItem *toItem)
{
    foreach (QSGItem *childItem, fromItem->childItems()) {
        if (childItem) {
            if (isAnchoredTo(childItem, toItem))
                return true;

            if (areChildrenAnchoredTo(childItem, toItem))
                return true;
        }
    }

    return false;
}

QSGAnchors *anchors(QSGItem *item)
{
    QSGItemPrivate *itemPrivate = static_cast<QSGItemPrivate*>(QSGItemPrivate::get(item));
    return itemPrivate->anchors();
}

QSGAnchors::Anchor anchorLineFlagForName(const QString &name)
{
    if (name == QLatin1String("anchors.top"))
        return QSGAnchors::TopAnchor;

    if (name == QLatin1String("anchors.left"))
        return QSGAnchors::LeftAnchor;

    if (name == QLatin1String("anchors.bottom"))
         return QSGAnchors::BottomAnchor;

    if (name == QLatin1String("anchors.right"))
        return QSGAnchors::RightAnchor;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return QSGAnchors::HCenterAnchor;

    if (name == QLatin1String("anchors.verticalCenter"))
         return QSGAnchors::VCenterAnchor;

    if (name == QLatin1String("anchors.baseline"))
         return QSGAnchors::BaselineAnchor;


    Q_ASSERT_X(false, Q_FUNC_INFO, "wrong anchor name - this should never happen");
    return QSGAnchors::LeftAnchor;
}

bool DesignerSupport::hasAnchor(QSGItem *item, const QString &name)
{
    if (!isValidAnchorName(name))
        return false;

    if (name == QLatin1String("anchors.fill"))
        return anchors(item)->fill() != 0;

    if (name == QLatin1String("anchors.centerIn"))
        return anchors(item)->centerIn() != 0;

    if (name == QLatin1String("anchors.right"))
        return anchors(item)->right().item != 0;

    if (name == QLatin1String("anchors.top"))
        return anchors(item)->top().item != 0;

    if (name == QLatin1String("anchors.left"))
        return anchors(item)->left().item != 0;

    if (name == QLatin1String("anchors.bottom"))
        return anchors(item)->bottom().item != 0;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return anchors(item)->horizontalCenter().item != 0;

    if (name == QLatin1String("anchors.verticalCenter"))
        return anchors(item)->verticalCenter().item != 0;

    if (name == QLatin1String("anchors.baseline"))
        return anchors(item)->baseline().item != 0;

    return anchors(item)->usedAnchors().testFlag(anchorLineFlagForName(name));
}

QSGItem *DesignerSupport::anchorFillTargetItem(QSGItem *item)
{
    return anchors(item)->fill();
}

QSGItem *DesignerSupport::anchorCenterInTargetItem(QSGItem *item)
{
    return anchors(item)->centerIn();
}



QPair<QString, QObject*> DesignerSupport::anchorLineTarget(QSGItem *item, const QString &name, QDeclarativeContext *context)
{
    QObject *targetObject = 0;
    QString targetName;

    if (name == QLatin1String("anchors.fill")) {
        targetObject = anchors(item)->fill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        targetObject = anchors(item)->centerIn();
    } else {
        QDeclarativeProperty metaProperty(item, name, context);
        if (!metaProperty.isValid())
            return QPair<QString, QObject*>();

        QSGAnchorLine anchorLine = metaProperty.read().value<QSGAnchorLine>();
        if (anchorLine.anchorLine != QSGAnchorLine::Invalid) {
            targetObject = anchorLine.item;
            targetName = propertyNameForAnchorLine(anchorLine.anchorLine);
        }

    }

    return QPair<QString, QObject*>(targetName, targetObject);
}

void DesignerSupport::resetAnchor(QSGItem *item, const QString &name)
{
    if (name == QLatin1String("anchors.fill")) {
        anchors(item)->resetFill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        anchors(item)->resetCenterIn();
    } else if (name == QLatin1String("anchors.top")) {
        anchors(item)->resetTop();
    } else if (name == QLatin1String("anchors.left")) {
        anchors(item)->resetLeft();
    } else if (name == QLatin1String("anchors.right")) {
        anchors(item)->resetRight();
    } else if (name == QLatin1String("anchors.bottom")) {
        anchors(item)->resetBottom();
    } else if (name == QLatin1String("anchors.horizontalCenter")) {
        anchors(item)->resetHorizontalCenter();
    } else if (name == QLatin1String("anchors.verticalCenter")) {
        anchors(item)->resetVerticalCenter();
    } else if (name == QLatin1String("anchors.baseline")) {
        anchors(item)->resetBaseline();
    }
}

QList<QObject*> DesignerSupport::statesForItem(QSGItem *item)
{
    QList<QObject*> objectList;
    QList<QDeclarativeState *> stateList = QSGItemPrivate::get(item)->_states()->states();
    qCopy(stateList.begin(), stateList.end(), objectList.begin());

    return objectList;
}

bool DesignerSupport::isComponentComplete(QSGItem *item)
{
    return static_cast<QSGItemPrivate*>(QSGItemPrivate::get(item))->componentComplete;
}

int DesignerSupport::borderWidth(QSGItem *item)
{
    QSGRectangle *rectangle = qobject_cast<QSGRectangle*>(item);
    if (rectangle)
        return rectangle->border()->width();

    return 0;
}

void DesignerSupport::refreshExpressions(QDeclarativeContext *context)
{
    QDeclarativeContextPrivate::get(context)->data->refreshExpressions();
}

void DesignerSupport::setRootItem(QSGView *view, QSGItem *item)
{
    QSGViewPrivate::get(view)->setRootObject(item);
}

bool DesignerSupport::isValidWidth(QSGItem *item)
{
    return QSGItemPrivate::get(item)->heightValid;
}

bool DesignerSupport::isValidHeight(QSGItem *item)
{
    return QSGItemPrivate::get(item)->widthValid;
}

void DesignerSupport::updateDirtyNode(QSGItem *item)
{
    QSGCanvasPrivate::get(item->canvas())->updateDirtyNode(item);
}

QT_END_NAMESPACE
