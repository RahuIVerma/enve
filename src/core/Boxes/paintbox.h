// enve - 2D animations software
// Copyright (C) 2016-2019 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PAINTBOX_H
#define PAINTBOX_H
#include "boundingbox.h"
#include "Paint/animatedsurface.h"
#include "imagebox.h"
class QPointFAnimator;
class AnimatedPoint;
class SimpleBrushWrapper;

struct PaintBoxRenderData : public ImageRenderData {
    e_OBJECT
public:
    PaintBoxRenderData(BoundingBox * const parentBoxT) :
        ImageRenderData(parentBoxT) {}

    void loadImageFromHandler() {
        if(fImage) return;
        if(fASurface) fASurface->getFrameImage(qFloor(fRelFrame), fImage);
    }

    void updateRelBoundingRect() final {
        Q_ASSERT(fSurface);
        fRelBoundingRect = fSurface->surface().pixelBoundingRect();
    }

    qptr<AnimatedSurface> fASurface;
    stdsptr<DrawableAutoTiledSurface> fSurface;
};

class PaintBox : public BoundingBox {
    e_OBJECT
protected:
    PaintBox();
public:
    bool SWT_isPaintBox() const { return true; }
    void setupRenderData(const qreal relFrame,
                         BoxRenderData * const data);
    stdsptr<BoxRenderData> createRenderData();

    void setupCanvasMenu(PropertyMenu * const menu);

    AnimatedSurface * getSurface() const {
        return mSurface.get();
    }
private:
    qsptr<AnimatedSurface> mSurface;
};

#endif // PAINTBOX_H
