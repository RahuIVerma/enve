#include "patheffect.h"
#include "pointhelpers.h"

PathEffect::PathEffect(const PathEffectType &type) {
    mPathEffectType = type;
}

DisplacePathEffect::DisplacePathEffect() :
    PathEffect(DISPLACE_PATH_EFFECT) {
    prp_setName("discrete effect");

    mSegLength->prp_setName("segment length");
    mSegLength->qra_setValueRange(0., 1000.);
    mSegLength->qra_setCurrentValue(20.);

    mMaxDev->prp_setName("max deviation");
    mMaxDev->qra_setValueRange(0., 1000.);
    mMaxDev->qra_setCurrentValue(20.);

    mSmoothness->prp_setName("smoothness");
    mSmoothness->qra_setValueRange(0., 1.);

    mRandomize->prp_setName("randomize");

    ca_addChildAnimator(mSegLength.data());
    ca_addChildAnimator(mMaxDev.data());
    ca_addChildAnimator(mSmoothness.data());
    ca_addChildAnimator(mRandomize.data());
}

Property *DisplacePathEffect::makeDuplicate() {
    DisplacePathEffect *newEffect = new DisplacePathEffect();
    makeDuplicate(newEffect);
    return newEffect;
}

void DisplacePathEffect::makeDuplicate(Property *target) {
    DisplacePathEffect *effectTarget = (DisplacePathEffect*)target;

    effectTarget->duplicateAnimatorsFrom(mSegLength.data(),
                                         mMaxDev.data());
}

void DisplacePathEffect::duplicateAnimatorsFrom(QrealAnimator *segLen,
                                                QrealAnimator *maxDev) {
    segLen->makeDuplicate(mSegLength.data());
    maxDev->makeDuplicate(mMaxDev.data());
}

static void Perterb(SkPoint* p,
                    const SkVector& tangent,
                    SkScalar scale) {
    SkVector normal = tangent;
    normal.rotateCCW();
    normal.setLength(scale);
    *p += normal;
}

class LCGRandom {
public:
    LCGRandom(uint32_t seed) : fSeed(seed) {}

    /** Return the next pseudo random number expressed as a SkScalar
        in the range [-SK_Scalar1..SK_Scalar1).
    */
    SkScalar nextSScalar1() { return SkFixedToScalar(this->nextSFixed1()); }

private:
    /** Return the next pseudo random number as an unsigned 32bit value.
    */
    uint32_t nextU() { uint32_t r = fSeed * kMul + kAdd; fSeed = r; return r; }

    /** Return the next pseudo random number as a signed 32bit value.
     */
    int32_t nextS() { return (int32_t)this->nextU(); }

    /** Return the next pseudo random number expressed as a signed SkFixed
     in the range [-SK_Fixed1..SK_Fixed1).
     */
    SkFixed nextSFixed1() { return this->nextS() >> 15; }

    //  See "Numerical Recipes in C", 1992 page 284 for these constants
    enum {
        kMul = 1664525,
        kAdd = 1013904223
    };
    uint32_t fSeed;
};

void getC1AndC2(const SkPoint &lastP,
                const SkPoint &currP,
                const SkPoint &nextP,
                SkPoint *c1, SkPoint *c2,
                const SkScalar &smoothLen) {
    SkPoint sLastP = lastP - currP;
    sLastP.setLength(1.f);
    SkPoint sNextP = nextP - currP;
    sNextP.setLength(1.f);
    SkPoint vectP = (sLastP + sNextP)*0.5f;
    vectP.set(vectP.y(), -vectP.x());
    if(vectP.dot(lastP - currP) > 0) vectP.negate();

    SkScalar nextDist = (currP - nextP).length()*0.4f;
    if(smoothLen < nextDist) {
        vectP.setLength(smoothLen);
    } else {
        vectP.setLength(nextDist);
    }

    *c1 = currP + vectP;

    vectP.negate();
    SkScalar lastDist = (currP - lastP).length()*0.5f;
    if(smoothLen < lastDist) {
        vectP.setLength(smoothLen);
    } else {
        vectP.setLength(lastDist);
    }
    *c2 = currP + vectP;
}

bool displaceFilterPath(SkPath* dst, const SkPath& src,
                        const SkScalar &maxDev,
                        const SkScalar &segLen,
                        const SkScalar &smoothness,
                        const uint32_t &seedAssist) {
    if(segLen < 0.01) return false;
    dst->reset();
    SkPathMeasure meas(src, false);

    /* Caller may supply their own seed assist, which by default is 0 */
    uint32_t seed = seedAssist ^ SkScalarRoundToInt(meas.getLength());

    LCGRandom rand(seed ^ ((seed << 16) | (seed >> 16)));
    SkScalar scale = maxDev;
    SkPoint p;
    SkVector v;
    if(smoothness < 0.001f) {
        do {
            SkScalar length = meas.getLength();

            if(segLen * (2/* + doFill*/) > length) {
                meas.getSegment(0, length, dst, true);  // to short for us to mangle
            } else {
                int n = SkScalarRoundToInt(length / segLen);
                SkScalar delta = length / n;
                SkScalar distance = 0;

                if(meas.isClosed()) {
                    n--;
                    distance += delta/2;
                }

                if(meas.getPosTan(distance, &p, &v)) {
                    Perterb(&p, v, rand.nextSScalar1() * scale);
                    dst->moveTo(p);
                }
                while(--n >= 0) {
                    distance += delta;
                    if(meas.getPosTan(distance, &p, &v)) {
                        Perterb(&p, v, rand.nextSScalar1() * scale);
                        dst->lineTo(p);
                    }
                }
                if(meas.isClosed()) {
                    dst->close();
                }
            }
        } while (meas.nextContour());
    } else {
        SkPoint firstP;
        SkPoint secondP;
        SkPoint thirdP;
        SkPoint lastP;
        SkPoint nextP;
        SkPoint currP;
        SkPoint lastC1;
        SkPoint c1;
        SkPoint c2;

        do {
            SkScalar smoothLen = smoothness * segLen * 0.5f;
            SkScalar length = meas.getLength();

            if(segLen * (2/* + doFill*/) > length) {
                meas.getSegment(0, length, dst, true);  // to short for us to mangle
            } else {
                int n = SkScalarRoundToInt(length / segLen);
                SkScalar delta = length / n;
                SkScalar distance = 0;

                if(meas.isClosed()) {
                    n--;
                    distance += delta/2;
                }

                if(meas.getPosTan(distance, &firstP, &v)) {
                    Perterb(&firstP, v, rand.nextSScalar1() * scale);
                    lastP = firstP;
                }

                if(meas.isClosed()) {
                    distance += delta;
                    if(meas.getPosTan(distance, &currP, &v)) {
                        Perterb(&currP, v, rand.nextSScalar1() * scale);
                        n--;
                        secondP = currP;
                    }
                    distance += delta;
                    if(meas.getPosTan(distance, &nextP, &v)) {
                        Perterb(&nextP, v, rand.nextSScalar1() * scale);
                        n--;
                        thirdP = nextP;

                        getC1AndC2(lastP, currP, nextP,
                                   &c1, &c2, smoothLen);

                        lastC1 = c1;

                        lastP = currP;
                        currP = nextP;
                    }
                } else {
                    currP = lastP;
                    lastC1 = currP;
                }
                dst->moveTo(lastP);
                while(--n >= 0) {
                    distance += delta;
                    if(meas.getPosTan(distance, &nextP, &v)) {
                        Perterb(&nextP, v, rand.nextSScalar1() * scale);
                        getC1AndC2(lastP, currP, nextP,
                                   &c1, &c2, smoothLen);


                        dst->cubicTo(lastC1, c2, currP);
                        lastC1 = c1;

                        lastP = currP;
                        currP = nextP;
                    }
                }

                nextP = firstP;
                getC1AndC2(lastP, currP, nextP,
                           &c1, &c2, smoothLen);
                dst->cubicTo(lastC1, c2, currP);
                if(meas.isClosed()) {
                    lastC1 = c1;

                    lastP = currP;
                    currP = nextP;
                    nextP = secondP;

                    getC1AndC2(lastP, currP, nextP,
                               &c1, &c2, smoothLen);

                    dst->cubicTo(lastC1, c2, currP);
                    lastC1 = c1;

                    lastP = currP;
                    currP = nextP;
                    nextP = thirdP;
                    getC1AndC2(lastP, currP, nextP,
                               &c1, &c2, smoothLen);
                    dst->cubicTo(lastC1, c2, currP);

                    dst->close();
                }
            }
        } while (meas.nextContour());
    }
    return true;
}

void DisplacePathEffect::filterPath(const SkPath &src,
                                    SkPath *dst) {
    if(mRandomize->getValue()) {
        mSeedAssist = qrand() % 2000;
    }
    displaceFilterPath(dst, src,
                       mMaxDev->qra_getCurrentValue(),
                       mSegLength->qra_getCurrentValue(),
                       mSmoothness->qra_getCurrentValue(),
                       mSeedAssist);
}

void DisplacePathEffect::filterPathForRelFrame(const int &relFrame,
                                               const SkPath &src,
                                               SkPath *dst) {
    if(mRandomize->getValue()) {
        mSeedAssist = qrand() % 2000;
    }
    displaceFilterPath(dst, src,
                       mMaxDev->qra_getValueAtRelFrame(relFrame),
                       mSegLength->qra_getValueAtRelFrame(relFrame),
                       mSmoothness->qra_getValueAtRelFrame(relFrame),
                       mSeedAssist);
}

DuplicatePathEffect::DuplicatePathEffect() :
    PathEffect(DUPLICATE_PATH_EFFECT) {
    prp_setName("duplicate effect");

    mTranslation->prp_setName("translation");
    mTranslation->setCurrentPointValue(QPointF(10., 10.));

    ca_addChildAnimator(mTranslation.data());
}

Property *DuplicatePathEffect::makeDuplicate() {
    DuplicatePathEffect *newEffect = new DuplicatePathEffect();
    makeDuplicate(newEffect);
    return newEffect;
}

void DuplicatePathEffect::makeDuplicate(Property *target) {
    DuplicatePathEffect *effectTarget = (DuplicatePathEffect*)target;

    effectTarget->duplicateAnimatorsFrom(mTranslation.data());
}

void DuplicatePathEffect::duplicateAnimatorsFrom(QPointFAnimator *trans) {
    trans->makeDuplicate(mTranslation.data());
}

void DuplicatePathEffect::filterPath(const SkPath &src,
                                     SkPath *dst) {
    *dst = src;
    dst->addPath(src,
                 mTranslation->getXValue(),
                 mTranslation->getYValue());
}

void DuplicatePathEffect::filterPathForRelFrame(const int &relFrame,
                                                const SkPath &src,
                                                SkPath *dst) {
    *dst = src;
    dst->addPath(src,
                 mTranslation->getXValueAtRelFrame(relFrame),
                 mTranslation->getYValueAtRelFrame(relFrame));
}
#include "pathoperations.h"
#include "skqtconversions.h"
SumPathEffect::SumPathEffect(PathBox *parentPath) :
    PathEffect(SUM_PATH_EFFECT) {
    prp_setName("path sum effect");
    mParentPathBox = parentPath;
    ca_addChildAnimator(mBoxTarget.data());
}

void SumPathEffect::filterPathForRelFrame(const int &relFrame,
                                          const SkPath &src,
                                          SkPath *dst) {
    PathBox *pathBox = ((PathBox*)mBoxTarget->getTarget());
    if(pathBox == NULL) {
        *dst = src;
        return;
    }
    int absFrame = mParentPathBox->
            prp_relFrameToAbsFrame(relFrame);
    int pathBoxRelFrame = pathBox->
            prp_absFrameToRelFrame(absFrame);
    SkPath boxPath = pathBox->getPathAtRelFrame(
                pathBoxRelFrame);
    if(src.isEmpty()) {
        *dst = boxPath;
        return;
    }
    if(boxPath.isEmpty()) {
        *dst = src;
        return;
    }
    QMatrix pathBoxMatrix =
            pathBox->getTransformAnimator()->
                getCombinedTransformMatrixAtRelFrame(
                    pathBoxRelFrame);
    QMatrix parentBoxMatrix =
            mParentPathBox->getTransformAnimator()->
                getCombinedTransformMatrixAtRelFrame(
                    relFrame);
    boxPath.transform(
                QMatrixToSkMatrix(
                    pathBoxMatrix*parentBoxMatrix.inverted()));
    FullVectorPath addToPath;
    addToPath.generateFromPath(src);
    FullVectorPath addedPath;
    addedPath.generateFromPath(boxPath);
    addToPath.intersectWith(&addedPath,
                            true,
                            true);
    FullVectorPath targetPath;
    targetPath.getSeparatePathsFromOther(&addToPath);
    targetPath.getSeparatePathsFromOther(&addedPath);
    targetPath.generateSinglePathPaths();

    *dst = QPainterPathToSkPath(targetPath.getPath());
}
