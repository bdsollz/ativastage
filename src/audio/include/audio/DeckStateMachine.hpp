#pragma once

#include "audio/DeckState.hpp"

namespace audio {

// Enforces the legal transitions between deck states. Pure logic (no threads,
// no I/O) so it is fully unit-testable headless. Each event returns true if the
// transition was allowed (and applied) and false otherwise, leaving the state
// unchanged on a rejected event.
//
// Legal transitions:
//   load()          : Ready|Paused|Playing|FadingOut|Failed -> Loading
//   onLoaded()      : Loading                               -> Ready
//   onLoadError()   : Loading                               -> Failed
//   play()          : Ready|Paused                          -> Playing
//   pause()         : Playing                               -> Paused
//   stop()          : Playing|Paused|FadingOut              -> Ready
//   fadeOut()       : Playing                               -> FadingOut
//   onFadeComplete(): FadingOut                             -> Ready
//   fail()          : (any)                                 -> Failed
//   reset()         : Failed                                -> Ready
class DeckStateMachine {
public:
    DeckState state() const { return state_; }

    bool load() {
        switch (state_) {
            case DeckState::Ready:
            case DeckState::Paused:
            case DeckState::Playing:
            case DeckState::FadingOut:
            case DeckState::Failed:
                return set(DeckState::Loading);
            case DeckState::Loading:
                return false; // already loading
        }
        return false;
    }

    bool onLoaded()    { return from(DeckState::Loading, DeckState::Ready); }
    bool onLoadError() { return from(DeckState::Loading, DeckState::Failed); }

    bool play() {
        if (state_ == DeckState::Ready || state_ == DeckState::Paused) {
            return set(DeckState::Playing);
        }
        return false;
    }

    bool pause() { return from(DeckState::Playing, DeckState::Paused); }

    bool stop() {
        if (state_ == DeckState::Playing || state_ == DeckState::Paused ||
            state_ == DeckState::FadingOut) {
            return set(DeckState::Ready);
        }
        return false;
    }

    bool fadeOut() { return from(DeckState::Playing, DeckState::FadingOut); }

    bool onFadeComplete() { return from(DeckState::FadingOut, DeckState::Ready); }

    // A hard failure can happen from any state (e.g. device lost, decode crash).
    bool fail() { return set(DeckState::Failed); }

    // Recover a failed deck back to idle.
    bool reset() { return from(DeckState::Failed, DeckState::Ready); }

private:
    bool set(DeckState s) {
        state_ = s;
        return true;
    }
    bool from(DeckState expected, DeckState next) {
        if (state_ == expected) {
            return set(next);
        }
        return false;
    }

    DeckState state_ = DeckState::Ready;
};

} // namespace audio
