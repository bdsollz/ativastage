#pragma once

#include <string>

namespace audio {

// Player states exactly as specified in the report (Seção 8): the same set is
// exposed on AppState and therefore visible on desktop and mobile. Enum names
// are English (code invariant); pt-BR labels for the UI come from
// deckStateLabelPtBr().
enum class DeckState {
    Ready,      // PRONTO   — idle or loaded, ready to play
    Loading,    // CARREGANDO
    Playing,    // TOCANDO
    Paused,     // PAUSADO
    FadingOut,  // FADE OUT
    Failed      // FALHA
};

// pt-BR label for the operator UI / mobile (matches the report's table).
inline const char* deckStateLabelPtBr(DeckState s) {
    switch (s) {
        case DeckState::Ready:     return "PRONTO";
        case DeckState::Loading:   return "CARREGANDO";
        case DeckState::Playing:   return "TOCANDO";
        case DeckState::Paused:    return "PAUSADO";
        case DeckState::FadingOut: return "FADE OUT";
        case DeckState::Failed:    return "FALHA";
    }
    return "PRONTO";
}

} // namespace audio
