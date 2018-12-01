#ifndef SCROLLWIDGET_H
#define SCROLLWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "minimalscrollwidget.h"
#include "sharedpointerdefs.h"
#include "selfref.h"
class SingleWidget;
class ScrollWidgetVisiblePart;
class SingleWidgetAbstraction;
class SingleWidgetTarget;
class ScrollArea;

class ScrollWidget : public MinimalScrollWidget {
    Q_OBJECT
public:
    explicit ScrollWidget(ScrollArea *parent = nullptr);

    void updateHeight();
    void setMainTarget(SingleWidgetTarget *target);
    virtual void updateAbstraction();
    const int &getContentHeight() {
        return mContentHeight;
    }
private slots:
    void updateHeightAfterScrollAreaResize(const int &parentHeight);
protected:
    int mContentHeight = 0;
    virtual void createVisiblePartWidget();
    qptr<SingleWidgetTarget> mMainTarget;
    stdptr<SingleWidgetAbstraction>mMainAbstraction;
    ScrollWidgetVisiblePart *mVisiblePartWidget = nullptr;
};

#endif // SCROLLWIDGET_H
