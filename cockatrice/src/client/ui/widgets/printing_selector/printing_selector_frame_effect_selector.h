#ifndef COCKATRICE_PRINTING_SELECTOR_FRAME_EFFECT_SELECTOR_H
#define COCKATRICE_PRINTING_SELECTOR_FRAME_EFFECT_SELECTOR_H
#include "../../../../game/cards/card_info.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class PrintingSelectorFrameEffectSelector : public QWidget
{
    Q_OBJECT

signals:
    void frameEffectToggled(const QString &effect, bool checked);

public:
    explicit PrintingSelectorFrameEffectSelector(QWidget *parent);
    void initializeFrameEffects();
    QStringList checkedFrameEffects() const;

public slots:
    void updateVisibleFrameEffects(const CardInfoPtr &card);

private:
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QWidget *container;
    QVBoxLayout *vbox;
};

#endif // COCKATRICE_PRINTING_SELECTOR_FRAME_EFFECT_SELECTOR_H
