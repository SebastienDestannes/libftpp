/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   data_structures.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sdestann <sdestann@student.42perpignan.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:12:12 by sdestann          #+#    #+#             */
/*   Updated: 2024/11/18 16:56:02 by sdestann         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DATA_STRUCTURES_HPP
# define DATA_STRUCTURES_HPP

#include <vector>
#include <queue>
#include <memory>
#include <cstring>
#include <stdexcept>

template<typename TType>
class Pool {
    /**
     * @brief La Pool a plusieurs avantages par rapport à un vector/list 
     * d'objects:
     * 
     * Performance : Elle évite les allocations/désallocations fréquentes de 
     * mémoire en réutilisant les objets créés.
     * Thread-safety : Les objets sont gérés de manière sûre avec plusieurs
     * threads.
     * Automatisation : L'Object gère automatiquement le retour au pool.
     * Optimisation : Idéal pour les objets créés/détruits fréquemment (ex:
     * particules dans un jeu, connexions réseau).
     * 
     * Exemple d'un pool de connexions de base de données :
     * 
     * class DatabaseConnection {
     *     public:
     *         DatabaseConnection() : isConnected(false) {}
     *         void connect(const std::string& host, const std::string& user) {
     *            Simule une connexion coûteuse
     *            std::this_thread::sleep_for(std::chrono::seconds(1));
     *            isConnected = true;
     *         }
     * 
     *     bool isConnected;
     * };
     * 
     * Utilisation :
     * 
     * int main() {
     *     Pool<DatabaseConnection> connectionPool;
     *     connectionPool.resize(5);  // Pré-alloue 5 connexions
     *     {
     *         Obtient une connexion du pool
     *         auto conn1 = connectionPool.acquire();
     *         conn1->connect("localhost", "user1");
     * 
     *         La connexion retourne au pool à la fin du scope
     *     }
     * 
     *     La même connexion peut être réutilisée
     *     auto conn2 = connectionPool.acquire();
     *     conn2 pointe vers la même connexion que conn1 précédemment
     * }
     * 
     * @throws std::runtime_error "Pool is empty" - Quand acquire() est appelé
     * sur un pool vide
     */
    
private:
    // Stockage des objets pré-alloués
    std::vector<std::unique_ptr<TType>> storage;
    // File des indices disponibles, si un objet est "prété", il est retiré de
    // la file.
    std::queue<size_t> available;

public:
    // Classe interne qui gère un objet du pool
    class Object {
    private:
        Pool<TType>* pool;
        size_t index;
        
    public:
        // Constructeur avec le pool et l'index de l'objet
        Object(Pool<TType>* p, size_t i) : pool(p), index(i) {}
        // Destructeur
        ~Object() {
            if (pool) {
                // Retourne l'objet au pool pour qu'il soit à nouveau available 
                // dans la file
                pool->available.push(index);
            }
        }
        
        // Opérateur -> pour accéder à l'objet
        TType* operator->() {
            return pool->storage[index].get();
        }
        
        // Empêcher la copie car chaque Object doit être unique (un même objet 
        // du pool ne peut pas être "possédé" par deux Object)
        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;
        
        // Permettre le déplacement pour pouvoir retourner un Object depuis une 
        // fonction
        Object(Object&& other) noexcept : pool(other.pool), index(other.index) {
            other.pool = nullptr;
        }
    };

    // La fonction resize permet de pré-allouer un certain nombre d'objets.
    void resize(const size_t& numberOfObjectStored) {
        storage.reserve(numberOfObjectStored);
        while (storage.size() < numberOfObjectStored) {
            storage.push_back(std::make_unique<TType>());
            available.push(storage.size() - 1);
        }
    }

    /**
     * @brief Cette fonction permet d'obtenir un objet du pool en le
     * construisant avec les arguments fournis.
     * @throws std::runtime_error "Pool is empty" - Si plus d'objets disponibles
     */

    template<typename... TArgs>
    Object acquire(TArgs&&... p_args) {
        if (available.empty()) {
            throw std::runtime_error("Pool is empty");
        }
        
        size_t index = available.front();
        available.pop();
        
        // Reconstruction de l'objet avec les nouveaux arguments
        storage[index] = std::make_unique<TType>(std::forward<TArgs>(p_args)...);
        
        return Object(this, index);
    }
};

class DataBuffer {
    /** @brief Cette class permet de créer un format de sérialisation pour 
     * transférer des objets complexes. On peut s'en servir pour la
     * communication entre deux programmes. Pour que ceux-ci se comprennent, ils
     * doivent :
     * 
     * 1 - Avoir la même structure de données.
     * 2 - Utiliser le même format de sérialisation.
     * 3 - Respecter le même ordre de sérialisation/désérialisation.
     * 
     * C'est très utilisé dans :
     * 
     * la communication client/serveur.
     * la auvegarde/chargement de données.
     * la communication entre processus.
     * 
     * L'avantage est l'efficacité en termes de taille de données et de
     * performance par rapport à des formats texte comme JSON.
     * 
     * @throws std::runtime_error "Buffer underflow" - Si buffer trop petit
     */
private:
    std::vector<char> buffer;

public:
    // Opérateur pour sérialiser (transformer en binaire) des données dans le 
    // buffer.
    template<typename T>
    DataBuffer& operator<<(const T& data) {
        size_t currentSize = buffer.size();
        buffer.resize(currentSize + sizeof(T));
        std::memcpy(buffer.data() + currentSize, &data, sizeof(T));
        return *this;
    }

    // Opérateur pour désérialiser des données depuis le buffer.
    template<typename T>
    DataBuffer& operator>>(T& data) {
        if (buffer.size() < sizeof(T)) {
            throw std::runtime_error("Buffer underflow");
        }
        std::memcpy(&data, buffer.data(), sizeof(T));
        buffer.erase(buffer.begin(), buffer.begin() + sizeof(T));
        return *this;
    }

    // Surcharge d'opérateurs << et >> pour les strings (car leurs tailles sont 
    // variables).
    DataBuffer& operator<<(const std::string& str) {
        size_t len = str.length();
        *this << len;
        size_t currentSize = buffer.size();
        buffer.resize(currentSize + len);
        std::memcpy(buffer.data() + currentSize, str.data(), len);
        return *this;
    }
    
    DataBuffer& operator>>(std::string& str) {
        size_t len;
        *this >> len;
        if (buffer.size() < len) {
            throw std::runtime_error("Buffer underflow");
        }
        str.assign(buffer.data(), len);
        buffer.erase(buffer.begin(), buffer.begin() + len);
        return *this;
    }
};

#endif