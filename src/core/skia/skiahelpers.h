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

#ifndef SKIAHELPERS_H
#define SKIAHELPERS_H

#include "skiaincludes.h"
#include "../exceptions.h"
#include <QIODevice>
#include "../castmacros.h"
#include "../basicreadwrite.h"

namespace SkiaHelpers {
    sk_sp<SkImage> makeCopy(const sk_sp<SkImage>& img);
    SkBitmap makeCopy(const SkBitmap& btmp);

    SkImageInfo getPremulRGBAInfo(const int width,
                                  const int height);
    void saveImage(const QString& fileName,
                   const sk_sp<SkImage>& img,
                   const SkEncodedImageFormat& format = SkEncodedImageFormat::kPNG,
                   const int quality = 100);

    sk_sp<SkImage> transferDataToSkImage(SkBitmap& bitmap);

    void writeImg(const sk_sp<SkImage>& img, eWriteStream &dst);
    sk_sp<SkImage> readImg(eReadStream& src);

    SkBitmap readBitmap(eReadStream &src);
    void writeBitmap(const SkBitmap& bitmap, eWriteStream &dst);

    void writePixmap(const SkPixmap& pix, eWriteStream &dst);

    void drawOutlineOverlay(SkCanvas * const canvas,
                            const SkPath &path,
                            const float invScale);

    void drawOutlineOverlay(SkCanvas * const canvas,
                            const SkPath &path,
                            const float invScale,
                            const SkMatrix& transform);

    void drawOutlineOverlay(SkCanvas * const canvas,
                            const SkPath &path,
                            const float invScale,
                            const bool dashes,
                            const float intervalSize);

    void drawOutlineOverlay(SkCanvas * const canvas,
                            const SkPath &path,
                            const float invScale,
                            const SkMatrix& transform,
                            const bool dashes,
                            const float intervalSize);

    void forceLink();
}

#endif // SKIAHELPERS_H
