/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   design_patterns.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sdestann <sdestann@student.42perpignan.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 16:07:52 by sdestann          #+#    #+#             */
/*   Updated: 2024/11/18 17:07:12 by sdestann         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DESIGN_PATTERNS_HPP
#define DESIGN_PATTERNS_HPP

#include "libftpp.hpp"
#include <map>
#include <vector>
#include <functional>
#include <stdexcept>

class Memento {
    /** @brief La class Memento permet de sauvegarder et de restaurer l'état d'un objet.
     * 
     * exemple :
     * 
     * class Player : public Memento {
     *     private:
     *         int health;
     *         std::string name;
     * 
     * 
     *         void _saveToSnapshot(Snapshot& snap) override {
     *             snap << health << name;  // Sauvegarde l'état
     *         }
     *         void _loadFromSnapshot(Snapshot& snap) override {
     *             snap >> health >> name;  // Restaure l'état
     *         }
     * };
     */
protected:
    class Snapshot {
        friend class Memento;
    private:
        DataBuffer buffer;
    public:
        template<typename T>
        Snapshot& operator<<(const T& data) {
            buffer << data;
            return *this;
        }

        template<typename T>
        Snapshot& operator>>(T& data) {
            buffer >> data;
            return *this;
        }
    };

private:
    virtual void _saveToSnapshot(Snapshot& snapshot) = 0;
    virtual void _loadFromSnapshot(Snapshot& snapshot) = 0;

public:
    Snapshot save() {
        Snapshot snapshot;
        _saveToSnapshot(snapshot);
        return snapshot;
    }

    void load(const Snapshot& state) {
        Snapshot snapshot = state;
        _loadFromSnapshot(snapshot);
    }

    virtual ~Memento() = default;
};

template<typename TEvent>
class Observer {
    /** La class Observer fonctionne comme un système de notifications :
     * Des objets (subscribers) s'inscrivent pour "observer" certains événements
     * (events) et d'envoyer une notification (notify) lorsqu'un événement se
     * produit.
     * 
     * exemple :
     * enum class GameEvent { PLAYER_DIED, LEVEL_UP };
     * 
     * Observer<GameEvent> gameObserver;
     * gameObserver.subscribe(GameEvent::PLAYER_DIED, []{ std::cout << 
     * "Game Over!"; });
     * gameObserver.notify(GameEvent::PLAYER_DIED); // Affiche "Game Over!"
     */
    
private:
    std::map<TEvent, std::vector<std::function<void()>>> subscribers;

public:
    void subscribe(const TEvent& event, const std::function<void()>& lambda) {
        subscribers[event].push_back(lambda);
    }

    void notify(const TEvent& event) {
        if (subscribers.find(event) != subscribers.end()) {
            for (const auto& lambda : subscribers[event]) {
                lambda();
            }
        }
    }
};

template<typename TType>
class Singleton {
    /** @brief la class Singleton permet de s'assurer qu'une seule instance
     * d'une class existe.
     * 
     * Exemple 1: Configuration globale
     * 
     * class Config : public Singleton<Config> {
     *     friend class Singleton<Config>;
     * private:
     *     Config() = default;
     *     std::string databaseUrl;
     * public:
     *     void setDatabaseUrl(const std::string& url) { databaseUrl = url; }
     * };
     * 
     * Exemple 2: Logger unique
     * class Logger : public Singleton<Logger> {
     *     friend class Singleton<Logger>;
     * private:
     *     Logger() = default;
     * public:
     *     void log(const std::string& msg) { ... }
     * };
     * 
     */

private:
    static TType* instancePtr;

public:
    static TType* instance() {
        return instancePtr;
    }

    template<typename... TArgs>
    static void instanciate(TArgs&&... p_args) {
        if (instancePtr) {
            throw std::runtime_error("Instance already exists");
        }
        instancePtr = new TType(std::forward<TArgs>(p_args)...);
    }
};

// initialisation du membre statique instancePtr. 
// En C++, les membres statiques doivent être définis en dehors de la classe.
template<typename TType>
TType* Singleton<TType>::instancePtr = nullptr;

template<typename TState>
class StateMachine {
    /** @brief StateMachine gère les états d'un système et les transitions entre
     * ces états.
     * 
     * exemple :
     * 
     * enum class PlayerState { IDLE, WALKING, RUNNING };
     * 
     * StateMachine<PlayerState> playerFSM;
     * playerFSM.addState(PlayerState::IDLE);
     * playerFSM.addState(PlayerState::WALKING);
     * 
     * Définit ce qui se passe lors de la transition IDLE -> WALKING.
     * playerFSM.addTransition(PlayerState::IDLE, PlayerState::WALKING,
     *    []{ std::cout << "Starting to walk"; });
     * 
     * Définit ce qui se passe pendant l'état WALKING.
     * playerFSM.addAction(PlayerState::WALKING,
     *   []{ std::cout << "Walking..."; });
     * 
     * Change l'état.
     * playerFSM.transitionTo(PlayerState::WALKING);
     * 
     * Exécute l'action de l'état actuel.
     * playerFSM.update();
     * 
     * La machine garde trace de :
     * 
     * 1 - États possibles.
     * 2 - Actions dans chaque état.
     * 3 - Transitions autorisées entre états.
     * 4 - État actuel.
     * 
     * @throws std::runtime_error "State not registered" - État non enregistré
     * @throws std::runtime_error "Invalid transition" - Transition non définie
     * @throws std::runtime_error "No action for current state" - Action
     * manquante
     */
    
private:
    TState currentState;
    std::map<TState, std::function<void()>> stateActions;
    std::map<std::pair<TState, TState>, std::function<void()>> transitions;
    std::map<TState, bool> states;

public:
    void addState(const TState& state) {
        states[state] = true;
    }

    void addTransition(const TState& startState, const TState& finalState, 
                      const std::function<void()>& lambda) {
        if (!states[startState] || !states[finalState]) {
            throw std::runtime_error("State not registered");
        }
        transitions[{startState, finalState}] = lambda;
    }

    void addAction(const TState& state, const std::function<void()>& lambda) {
        if (!states[state]) {
            throw std::runtime_error("State not registered");
        }
        stateActions[state] = lambda;
    }

    void transitionTo(const TState& state) {
        auto transitionIt = transitions.find({currentState, state});
        if (transitionIt == transitions.end()) {
            throw std::runtime_error("Invalid transition");
        }
        transitionIt->second();
        currentState = state;
    }

    void update() {
        auto actionIt = stateActions.find(currentState);
        if (actionIt == stateActions.end()) {
            throw std::runtime_error("No action for current state");
        }
        actionIt->second();
    }

    // Getters qui ne sont pas demandés par le sujets mais qu'il m'a semblé 
    // judicieux d'ajouter.
    
    TState getCurrentState() const { return currentState; }
    
    bool hasState(const TState& state) const {
        return states.find(state) != states.end();
    }
    
    bool hasTransition(const TState& from, const TState& to) const {
        return transitions.find({from, to}) != transitions.end();
    }
    
    bool hasAction(const TState& state) const {
        return stateActions.find(state) != stateActions.end();
    }
};


#endif