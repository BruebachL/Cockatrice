#ifndef COCKATRICE_NOOP_CARD_PREFERENCE_PROVIDER_H
#define COCKATRICE_NOOP_CARD_PREFERENCE_PROVIDER_H
#include "interface_card_preference_provider.h"

class NoopCardPreferenceProvider : public ICardPreferenceProvider {
public:
    QString getCardPreferenceOverride(const QString &) const override {
        return {};
    }
};

#endif //COCKATRICE_NOOP_CARD_PREFERENCE_PROVIDER_H
