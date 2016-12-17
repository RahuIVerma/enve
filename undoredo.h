#ifndef UNDOREDO_H
#define UNDOREDO_H

#include <QList>
#include "canvas.h"
#include "vectorpath.h"
#include "movablepoint.h"
#include "qrealanimator.h"
#include <QDebug>

class UndoRedo
{
public:
    UndoRedo(QString name);
    virtual ~UndoRedo() {}
    virtual void undo() {}
    virtual void redo() {}
    void printName() { qDebug() << mName; }
    void printUndoName() { qDebug() << "UNDO " << mName; }
    void printRedoName() { qDebug() << "REDO " << mName; }

    int getFrame() { return mFrame; }
private:
    int mFrame;
    QString mName;
};

class UndoRedoSet : public UndoRedo
{
public:
    UndoRedoSet() : UndoRedo("UndoRedoSet") {
    }

    ~UndoRedoSet() {
        foreach(UndoRedo *undoRedo, mSet) {
            delete undoRedo;
        }
    }

    void undo() {
        for(int i = mSet.length() - 1; i >= 0; i--) {
            mSet.at(i)->undo();
        }
    }

    void redo() {
        foreach(UndoRedo* undoRedo, mSet) {
            undoRedo->redo();
        }
    }

    void addUndoRedo(UndoRedo* undoRedo) {
        mSet << undoRedo;
    }

    bool isEmpty() {
        return mSet.isEmpty();
    }

private:
    QList<UndoRedo*> mSet;
};


class UndoRedoStack {
public:
    UndoRedoStack() {
        startNewSet();
    }

    void startNewSet() {
        mNumberOfSets++;
    }

    ~UndoRedoStack() {
        clearUndoStack();
        clearRedoStack();
    }

    void setWindow(MainWindow *mainWindow);

    void finishSet() {
        mNumberOfSets--;
        if(mNumberOfSets == 0) {
            addSet();
        }
    }

    void addSet() {
        if((mCurrentSet == NULL) ? true : mCurrentSet->isEmpty()) {
            mCurrentSet = NULL;
            return;
        }
        addUndoRedo(mCurrentSet);
        mCurrentSet = NULL;
    }

    void addToSet(UndoRedo *undoRedo) {
        if(mCurrentSet == NULL) {
            mCurrentSet = new UndoRedoSet();
        }
        mCurrentSet->addUndoRedo(undoRedo);
    }

    void clearRedoStack() {
        foreach (UndoRedo *redoStackItem, mRedoStack) {
            delete redoStackItem;
        }
        mRedoStack.clear();
    }

    void clearUndoStack() {
        foreach (UndoRedo *undoStackItem, mUndoStack) {
            delete undoStackItem;
        }
        mUndoStack.clear();
    }

    void emptySomeOfUndo() {
        if(mUndoStack.length() > 150) {
//            for(int i = 0; i < 50; i++) {
                delete mUndoStack.takeFirst();
//            }
        }
    }

    void clearAll() {
        clearRedoStack();
        clearUndoStack();
    }

    void addUndoRedo(UndoRedo *undoRedo);

    void redo();

    void undo();

private:
    int mLastUndoRedoFrame = INT_MAX;
    MainWindow *mMainWindow;
    int mNumberOfSets = 0;
    UndoRedoSet *mCurrentSet = NULL;
    QList<UndoRedo*> mUndoStack;
    QList<UndoRedo*> mRedoStack;
};

class MoveMovablePointUndoRedo : public UndoRedo
{
public:
    MoveMovablePointUndoRedo(MovablePoint *movedPoint,
                         QPointF relPosBefore,
                         QPointF relPosAfter) : UndoRedo("MoveMovablePointUndoRedo") {
        mMovedPoint = movedPoint;
        mMovedPoint->incNumberPointers();
        mRelPosAfter = relPosAfter;
        mRelPosBefore = relPosBefore;
    }

    ~MoveMovablePointUndoRedo() {
        mMovedPoint->decNumberPointers();
    }

    void redo() {
        mMovedPoint->setRelativePos(mRelPosAfter, false);
    }

    void undo() {
        mMovedPoint->setRelativePos(mRelPosBefore, false);
    }

private:
    MovablePoint *mMovedPoint;
    QPointF mRelPosBefore;
    QPointF mRelPosAfter;
};

class AppendToPointsListUndoRedo : public UndoRedo
{
public:
    AppendToPointsListUndoRedo(PathPoint *pointToAdd, VectorPath *path) : UndoRedo("AppendToPointsListUndoRedo") {
        mPoint = pointToAdd;
        mPath = path;
        mPoint->incNumberPointers();
        mPath->incNumberPointers();
    }

    ~AppendToPointsListUndoRedo() {
        mPath->decNumberPointers();
        mPoint->decNumberPointers();
    }

    void redo() {
        mPath->appendToPointsList(mPoint, false);
    }

    void undo() {
        mPath->removeFromPointsList(mPoint, false);
    }

private:
    PathPoint *mPoint;
    VectorPath *mPath;
};

class RemoveFromPointsListUndoRedo : public AppendToPointsListUndoRedo
{
public:
    RemoveFromPointsListUndoRedo(PathPoint *pointToRemove, VectorPath *path) : AppendToPointsListUndoRedo(pointToRemove, path) {
    }

    void redo() {
        AppendToPointsListUndoRedo::undo();
    }

    void undo() {
        AppendToPointsListUndoRedo::redo();
    }
};

class SetNextPointUndoRedo : public UndoRedo
{
public:
    SetNextPointUndoRedo(PathPoint *point, PathPoint *oldNext, PathPoint *newNext) : UndoRedo("SetNextPointUndoRedo") {
        mNewNext = newNext;
        mOldNext = oldNext;
        mPoint = point;
        if(mNewNext != NULL) mNewNext->incNumberPointers();
        if(mOldNext != NULL) mOldNext->incNumberPointers();
        mPoint->incNumberPointers();
    }

    ~SetNextPointUndoRedo() {
        if(mNewNext != NULL) mNewNext->decNumberPointers();
        if(mOldNext != NULL) mOldNext->decNumberPointers();
        mPoint->decNumberPointers();
    }

    void redo(){
        mPoint->setNextPoint(mNewNext, false);
    }

    void undo() {
        mPoint->setNextPoint(mOldNext, false);
    }

private:
    PathPoint *mNewNext;
    PathPoint *mOldNext;
    PathPoint *mPoint;
};

class SetPreviousPointUndoRedo : public UndoRedo
{
public:
    SetPreviousPointUndoRedo(PathPoint *point, PathPoint *oldPrevious, PathPoint *newPrevious) : UndoRedo("SetPreviousPointUndoRedo") {
        mNewPrev = newPrevious;
        mOldPrev = oldPrevious;
        mPoint = point;
        if(mNewPrev != NULL) mNewPrev->incNumberPointers();
        if(mOldPrev != NULL) mOldPrev->incNumberPointers();
        mPoint->incNumberPointers();
    }

    ~SetPreviousPointUndoRedo() {
        if(mNewPrev != NULL) mNewPrev->decNumberPointers();
        if(mOldPrev != NULL) mOldPrev->decNumberPointers();
        mPoint->decNumberPointers();
    }

    void redo(){
        mPoint->setPreviousPoint(mNewPrev, false);
    }

    void undo() {
        mPoint->setPreviousPoint(mOldPrev, false);
    }

private:
    PathPoint *mNewPrev;
    PathPoint *mOldPrev;
    PathPoint *mPoint;
};

class AddPointToSeparatePathsUndoRedo : public UndoRedo
{
public:
    AddPointToSeparatePathsUndoRedo(VectorPath *path, PathPoint *point) : UndoRedo("AddPointToSeparatePathsUndoRedo") {
        mPath = path;
        mPath->incNumberPointers();
        mPoint = point;
        mPoint->incNumberPointers();
    }

    ~AddPointToSeparatePathsUndoRedo() {
        mPath->decNumberPointers();
        mPoint->decNumberPointers();
    }

    void redo() {
        mPath->addPointToSeparatePaths(mPoint, false);
    }

    void undo() {
        mPath->removePointFromSeparatePaths(mPoint, false);
    }

private:
    VectorPath *mPath;
    PathPoint *mPoint;
};

class RemovePointFromSeparatePathsUndoRedo : public AddPointToSeparatePathsUndoRedo
{
public:
    RemovePointFromSeparatePathsUndoRedo(VectorPath *path, PathPoint *point) : AddPointToSeparatePathsUndoRedo(path, point) {

    }

    void redo() {
        AddPointToSeparatePathsUndoRedo::undo();
    }

    void undo() {
        AddPointToSeparatePathsUndoRedo::redo();
    }
};

class SetPathPointModeUndoRedo : public UndoRedo
{
public:
    SetPathPointModeUndoRedo(PathPoint *point, CtrlsMode modeBefore, CtrlsMode modeAfter) : UndoRedo("SetPathPointModeUndoRedo") {
        mPoint = point;
        mPoint->incNumberPointers();
        mBefore = modeBefore;
        mAfter = modeAfter;
    }

    ~SetPathPointModeUndoRedo() {
        mPoint->decNumberPointers();
    }

    void redo() {
        mPoint->setCtrlsMode(mAfter, false);
    }

    void undo() {
        mPoint->setCtrlsMode(mBefore, false);
    }

private:
    PathPoint *mPoint;
    CtrlsMode mBefore;
    CtrlsMode mAfter;
};

class SetCtrlPtEnabledUndoRedo : public UndoRedo
{
public:
    SetCtrlPtEnabledUndoRedo(bool enabled, bool isStartPt, PathPoint* parentPoint) : UndoRedo("SetCtrlPtEnabledUndoRedo") {
        mParentPoint = parentPoint;
        mParentPoint->incNumberPointers();
        mEnabled = enabled;
        mIsStartPt = isStartPt;
    }

    ~SetCtrlPtEnabledUndoRedo() {
        mParentPoint->decNumberPointers();
    }

    void redo() {
        mParentPoint->setCtrlPtEnabled(mEnabled, mIsStartPt, false);
    }

    void undo() {
        mParentPoint->setCtrlPtEnabled(!mEnabled, mIsStartPt, false);
    }

private:
    PathPoint *mParentPoint;
    bool mEnabled;
    bool mIsStartPt;
};

class MoveChildInListUndoRedo : public UndoRedo
{
public:
    MoveChildInListUndoRedo(int fromIndex,
                            int toIndex,
                            BoxesGroup *parentBox) : UndoRedo("MoveChildInListUndoRedo") {
        mParentBox = parentBox;
        mParentBox->incNumberPointers();
        mFromIndex = fromIndex;
        mToIndex = toIndex;
    }

    ~MoveChildInListUndoRedo() {
        mParentBox->decNumberPointers();
    }

    void redo() {
        mParentBox->moveChildInList(mFromIndex, mToIndex, false);
    }

    void undo() {
        mParentBox->moveChildInList(mToIndex, mFromIndex, false);
    }
private:
    BoxesGroup *mParentBox;
    int mFromIndex;
    int mToIndex;
};

class SetBoundingBoxZListIndexUnoRedo : public UndoRedo
{
public:
    SetBoundingBoxZListIndexUnoRedo(int indexBefore,
                                    int indexAfter,
                                    BoundingBox *box) : UndoRedo("SetBoundingBoxZListIndexUnoRedo") {
        mBox = box;
        mBox->incNumberPointers();
        mIndexAfter = indexAfter;
        mIndexBefore = indexBefore;
    }

    ~SetBoundingBoxZListIndexUnoRedo() {
        mBox->decNumberPointers();
    }

    void redo() {
        mBox->setZListIndex(mIndexAfter, false);
    }

    void undo() {
        mBox->setZListIndex(mIndexBefore, false);
    }

private:
    int mIndexBefore;
    int mIndexAfter;
    BoundingBox *mBox;
};

class AddChildToListUndoRedo : public UndoRedo
{
public:
    AddChildToListUndoRedo(BoxesGroup *parent,
                           int index,
                           BoundingBox *child) : UndoRedo("AddChildToListUndoRedo") {
        mParent = parent;
        mParent->incNumberPointers();
        mAddAtId = index;
        mChild = child;
        mChild->incNumberPointers();
    }

    ~AddChildToListUndoRedo() {
        mChild->decNumberPointers();
        mParent->decNumberPointers();
    }

    void redo() {
        mParent->addChildToListAt(mAddAtId, mChild, false);
    }

    void undo() {
        mParent->removeChildFromList(mAddAtId, false);
    }

private:
    BoxesGroup *mParent;
    int mAddAtId;
    BoundingBox *mChild;
};

class RemoveChildFromListUndoRedo : public AddChildToListUndoRedo
{
public:
    RemoveChildFromListUndoRedo(BoxesGroup *parent,
                                int index,
                                BoundingBox *child) :
        AddChildToListUndoRedo(parent, index, child) {

    }

    void redo() {
        AddChildToListUndoRedo::undo();
    }

    void undo() {
        AddChildToListUndoRedo::redo();
    }
private:
};

class SetBoxParentUndoRedo : public UndoRedo
{
public:
    SetBoxParentUndoRedo(BoundingBox *childBox,
                         BoxesGroup *oldParent,
                         BoxesGroup *newParent) : UndoRedo("SetBoxParentUndoRedo")
    {
        mChildBox = childBox;
        if(childBox != NULL) mChildBox->incNumberPointers();
        mOldParent = oldParent;
        if(oldParent != NULL) mOldParent->incNumberPointers();
        mNewParent = newParent;
        if(newParent != NULL) mNewParent->incNumberPointers();
    }

    ~SetBoxParentUndoRedo() {
        if(mChildBox != NULL) mChildBox->decNumberPointers();
        if(mOldParent != NULL) mOldParent->decNumberPointers();
        if(mNewParent != NULL) mNewParent->decNumberPointers();
    }

    void redo() {
        mChildBox->setParent(mNewParent, false);
    }

    void undo() {
        mChildBox->setParent(mOldParent, false);
    }

private:
    BoundingBox *mChildBox;
    BoxesGroup *mOldParent;
    BoxesGroup *mNewParent;
};

//class StrokeSettingsChangedUndoRedo : public UndoRedo
//{
//public:
//    StrokeSettingsChangedUndoRedo(StrokeSettings oldStrokeSettings,
//                                  StrokeSettings newStrokeSettings,
//                                  VectorPath *target) : UndoRedo("StrokeSettingsChangedUndoRedo") {
//        mOldSettings = oldStrokeSettings;
//        mNewSettings = newStrokeSettings;
//        mTarget = target;
//        mTarget->incNumberPointers();
//    }

//    ~StrokeSettingsChangedUndoRedo() {
//        mTarget->decNumberPointers();
//    }

//    void updateDisplayedSettings() {
//        if(mTarget->isSelected()) {
//            mTarget->getParent()->
//                    setCurrentFillStrokeSettingsFromBox(mTarget);
//        }
//    }

//    void redo() {
//        mTarget->setStrokeSettings(mNewSettings, false);
//        updateDisplayedSettings();
//    }

//    void undo() {
//        mTarget->setStrokeSettings(mOldSettings, false);
//        updateDisplayedSettings();
//    }

//private:
//    StrokeSettings mOldSettings;
//    StrokeSettings mNewSettings;
//    VectorPath *mTarget;
//};

//class FillSettingsChangedUndoRedo : public UndoRedo
//{
//public:
//    FillSettingsChangedUndoRedo(VectorPath *target) : UndoRedo("FillSettingsChangedUndoRedo") {
//        mTarget = target;
//        mTarget->incNumberPointers();
//    }

//    ~FillSettingsChangedUndoRedo() {
//        mTarget->decNumberPointers();
//    }

//    void updateDisplayedSettings() {
//        if(mTarget->isSelected()) {
//            mTarget->getParent()->
//                    setCurrentFillStrokeSettingsFromBox(mTarget);
//        }
//    }

//    void redo() {
//        mTarget->scheduleAwaitUpdate();
//        updateDisplayedSettings();
//    }

//    void undo() {
//        mTarget->scheduleAwaitUpdate();
//        updateDisplayedSettings();
//    }

//private:
//    VectorPath *mTarget;
//};

class ChangeGradientColorsUndoRedo : public UndoRedo
{
public:
    ChangeGradientColorsUndoRedo(QList<Color> oldColors,
                                 QList<Color> newColors,
                                 Gradient *gradient) : UndoRedo("ChangeGradientColorsUndoRedo") {
        mGradient = gradient;
        mGradient->incNumberPointers();
        mOldColors = oldColors;
        mNewColors = newColors;
    }

    ~ChangeGradientColorsUndoRedo() {
        mGradient->decNumberPointers();
    }

    void redo() {
        mGradient->setColors(mNewColors);
    }

    void undo() {
        mGradient->setColors(mOldColors);
    }

private:
    Gradient *mGradient;
    QList<Color> mOldColors;
    QList<Color> mNewColors;
};

class SetPivotRelPosUndoRedo : public UndoRedo
{
public:
    SetPivotRelPosUndoRedo(BoundingBox *target, QPointF prevRelPos, QPointF newRelPos,
                           bool prevPivotChanged, bool newPivotChanged) :
        UndoRedo("SetPivotRelPosUndoRedo") {
        mTarget = target;
        mTarget->incNumberPointers();
        mPrevRelPos = prevRelPos;
        mNewRelPos = newRelPos;
        mPrevPivotChanged = prevPivotChanged;
        mNewPivotChanged = newPivotChanged;
    }

    ~SetPivotRelPosUndoRedo() {
        mTarget->decNumberPointers();
    }

    void redo() {
        mTarget->setPivotRelPos(mNewRelPos, false, mNewPivotChanged);
    }

    void undo() {
        mTarget->setPivotRelPos(mPrevRelPos, false, mPrevPivotChanged);
    }

private:
    BoundingBox *mTarget;
    QPointF mPrevRelPos;
    QPointF mNewRelPos;
    bool mPrevPivotChanged;
    bool mNewPivotChanged;
};

class  SetBoxVisibleUndoRedo : public UndoRedo
{
public:
     SetBoxVisibleUndoRedo(BoundingBox *target,
                           bool visibleBefore, bool visibleAfter) :
         UndoRedo("SetBoxVisibleUndoRedo") {
         mTarget = target;
         mTarget->incNumberPointers();
         mVisibleAfter = visibleAfter;
         mVisibleBefore = visibleBefore;
     }

     ~SetBoxVisibleUndoRedo() {
         mTarget->decNumberPointers();
     }

     void redo() {
         mTarget->setVisibile(mVisibleAfter, false);
     }

     void undo() {
         mTarget->setVisibile(mVisibleBefore, false);
     }

private:
     bool mVisibleBefore;
     bool mVisibleAfter;
     BoundingBox *mTarget;
};

class ChangeQrealAnimatorValue : public UndoRedo
{
public:
    ChangeQrealAnimatorValue(qreal oldValue, qreal newValue,
                             QrealAnimator *animator) :
        UndoRedo("ChangeQrealAnimatorValue") {
        mOldValue = oldValue;
        mNewValue = newValue;
        mAnimator = animator;
        mAnimator->incNumberPointers();
    }

    ~ChangeQrealAnimatorValue() {
        mAnimator->decNumberPointers();
    }

    void redo() {
        mAnimator->setCurrentValue(mNewValue);
    }

    void undo() {
        mAnimator->setCurrentValue(mOldValue);
    }

private:
    qreal mOldValue;
    qreal mNewValue;
    QrealAnimator *mAnimator;
};

class ChangeQrealKeyValueUndoRedo : public UndoRedo
{
public:
    ChangeQrealKeyValueUndoRedo(qreal oldValue, qreal newValue, QrealKey *key) :
        UndoRedo("ChangeQrealKeyValueUndoRedo") {
        mOldValue = oldValue;
        mNewValue = newValue;
        mTargetKey = key;
        key->incNumberPointers();
    }

    ~ChangeQrealKeyValueUndoRedo() {
        mTargetKey->decNumberPointers();
    }

    void redo() {
        mTargetKey->getParentAnimator()->saveValueToKey(mTargetKey, mNewValue, false);
    }

    void undo() {
        mTargetKey->getParentAnimator()->saveValueToKey(mTargetKey, mOldValue, false);
    }

private:
    qreal mOldValue;
    qreal mNewValue;
    QrealKey *mTargetKey;
};

class ChangeQrealKeyFrameUndoRedo : public UndoRedo
{
public:
    ChangeQrealKeyFrameUndoRedo(int oldFrame, int newFrame, QrealKey *key) :
        UndoRedo("ChangeQrealKeyFrameUndoRedo") {
        mOldFrame = oldFrame;
        mNewFrame = newFrame;
        mTargetKey = key;
        key->incNumberPointers();
    }

    ~ChangeQrealKeyFrameUndoRedo() {
        mTargetKey->decNumberPointers();
    }

    void redo() {
        mTargetKey->getParentAnimator()->moveKeyToFrame(mTargetKey, mNewFrame);
    }

    void undo() {
        mTargetKey->getParentAnimator()->moveKeyToFrame(mTargetKey, mOldFrame);
    }

private:
    int mOldFrame;
    int mNewFrame;
    QrealKey *mTargetKey;
};

class QrealAnimatorRecordingSetUndoRedo : public UndoRedo
{
public:
    QrealAnimatorRecordingSetUndoRedo(bool recordingOld,
                                      bool recordingNew,
                                      QrealAnimator *animator) :
    UndoRedo("QrealAnimatorRecordingSetUndoRedo") {
        mRecordingOld = recordingOld;
        mRecordingNew = recordingNew;
        mAnimator = animator;
        mAnimator->incNumberPointers();
    }

    ~QrealAnimatorRecordingSetUndoRedo() {
        mAnimator->decNumberPointers();
    }

    void undo() {
        mAnimator->setRecordingWithoutChangingKeys(mRecordingOld, false);
    }

    void redo() {
        mAnimator->setRecordingWithoutChangingKeys(mRecordingNew, false);
    }

private:
    bool mRecordingOld;
    bool mRecordingNew;
    QrealAnimator *mAnimator;
};

class AddQrealKeyToAnimatorUndoRedo : public UndoRedo
{
public:
    AddQrealKeyToAnimatorUndoRedo(QrealKey *key, QrealAnimator *animator) :
        UndoRedo("AddQrealKeyToAnimatorUndoRedo") {
        mKey = key;
        mAnimator = animator;
        mAnimator->incNumberPointers();
        mKey->incNumberPointers();
    }

    ~AddQrealKeyToAnimatorUndoRedo() {
        mAnimator->decNumberPointers();
        mKey->decNumberPointers();
    }

    void redo() {
        mAnimator->appendKey(mKey, false);
    }

    void undo() {
        mAnimator->removeKey(mKey, false);
    }

private:
    QrealKey *mKey;
    QrealAnimator *mAnimator;
};

class RemoveQrealKeyFromAnimatorUndoRedo : public AddQrealKeyToAnimatorUndoRedo
{
public:
    RemoveQrealKeyFromAnimatorUndoRedo(QrealKey *key, QrealAnimator *animator) :
        AddQrealKeyToAnimatorUndoRedo(key, animator) {

    }

    void redo() {
        AddQrealKeyToAnimatorUndoRedo::undo();
    }

    void undo() {
        AddQrealKeyToAnimatorUndoRedo::redo();
    }
};

class RemoveShapeUndoRedo : public UndoRedo
{
public:
    RemoveShapeUndoRedo(VectorPath *targetPath, VectorPathShape *shape) :
        UndoRedo("RemoveShapeUndoRedo") {
        mTargetPath = targetPath;
        mShape = shape;
        mShape->incNumberPointers();
    }

    ~RemoveShapeUndoRedo() {
        mShape->decNumberPointers();
    }

    void redo() {
        mTargetPath->removeShape(mShape, false);
    }

    void undo() {
        mTargetPath->addShape(mShape, false);
    }

private:
    VectorPath *mTargetPath;
    VectorPathShape *mShape;
};

class AddShapeUndoRedo : public RemoveShapeUndoRedo
{
public:
    AddShapeUndoRedo(VectorPath *targetPath, VectorPathShape *shape) :
        RemoveShapeUndoRedo(targetPath, shape) {

    }

    void redo() {
        RemoveShapeUndoRedo::undo();
    }

    void undo() {
        RemoveShapeUndoRedo::redo();
    }
};

#endif // UNDOREDO_H
