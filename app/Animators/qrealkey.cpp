#include "Animators/qrealkey.h"
#include "Animators/qrealanimator.h"
#include "Animators/complexanimator.h"
#include "clipboardcontainer.h"
#include "Animators/qrealpoint.h"

QrealKey::QrealKey(const qreal &value, const int &frame,
                   QrealAnimator * const parentAnimator) :
    GraphKey(frame, parentAnimator) {
    mValue = value;
    mStartValue = mValue;
    mEndValue = mValue;
}

QrealKey::QrealKey(QrealAnimator * const parentAnimator) :
    QrealKey(0, 0, parentAnimator) { }

void QrealKey::incValue(const qreal &incBy) {
    setValue(mValue + incBy);
}

QrealAnimator *QrealKey::getParentQrealAnimator() const {
    return static_cast<QrealAnimator*>(mParentAnimator.data());
}

//bool QrealKey::isNear(qreal frameT, qreal valueT,
//                        qreal pixelsPerFrame, qreal pixelsPerValue) {
//    qreal value = getValue();
//    qreal frame = getFrame();
//    if(qAbs(frameT - frame)*pixelsPerFrame > mRadius) return false;
//    if(qAbs(valueT - value)*pixelsPerValue > mRadius) return false;
//    return true;
//}


qreal QrealKey::getValue() const { return mValue; }

void QrealKey::setValue(const qreal& value) {
    const qreal dVal = value - mValue;
    setEndValueVar(mEndValue + dVal);
    setStartValueVar(mStartValue + dVal);

    mValue = value;
    if(!this->mParentAnimator) return;
    mParentAnimator->anim_updateAfterChangedKey(this);
}

void QrealKey::finishValueTransform() {
    if(mParentAnimator)
        mParentAnimator->anim_updateAfterChangedKey(this);
}

void QrealKey::startValueTransform() {
    mSavedValue = mValue;
}

void QrealKey::saveCurrentFrameAndValue() {
    mSavedRelFrame = getRelFrame();
    mSavedValue = getValue();
}

bool QrealKey::differsFromKey(Key *key) const {
    if(key == this) return false;
    stdsptr<QrealKey> qa_key = GetAsSPtr(key, QrealKey);
    if(isZero4Dec(qa_key->getValue() - mValue)) {
        if(key->getRelFrame() > mRelFrame) {
            if(qa_key->getStartEnabledForGraph() || mEndEnabled) return true;
        } else {
            if(qa_key->getEndEnabledForGraph() || mStartEnabled) return true;
        }
        return false;
    }
    return true;
}

void QrealKey::changeFrameAndValueBy(const QPointF &frameValueChange) {
    int newFrame = qRound(frameValueChange.x() + mSavedRelFrame);
    bool frameChanged = newFrame != mRelFrame;
    setValue(frameValueChange.y() + mSavedValue);
    if(!frameChanged) return;
    if(mParentAnimator) {
        mParentAnimator->anim_moveKeyToRelFrame(this, newFrame);
    } else {
        setRelFrame(newFrame);
    }
}
