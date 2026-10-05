/*
 * IA.hpp
 * Copyright (C) 2026 prvgal [Pablo Rivero Galvín] <rivdevapps@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty统; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef IA_HPP_
#define IA_HPP_

#include "tipos.hpp"
#include "Juego.hpp"
#include <vector>
#include <optional>

class IA {
public:
    IA() = default;

    // inicializa la memoria de la IA para una nueva partida (asume 4 cartas iniciales)
    void reiniciarMemoria(size_t numCartas);

    // registra en su memoria una carta que ha podido consultar u observar
    void recordarCartaPropia(size_t pos, Carta carta);

    // Decidir qué hacer tras robar una carta (Intercambiar, Descartar o Usar Efecto)
    enum class AccionTurno {
        DESAGRADO_DESCARTAR,  // Descartar la carta robada sin mas
        USAR_EFECTO,          // Descartar la carta robada usando su habilidad (10, 11, 12)
        INTERCAMBIAR          // Intercambiar la carta robada por una de la mano
    };

    struct DecisionAccion {
        AccionTurno accion;
        size_t posManoIntercambio{0}; // Si la accion es INTERCAMBIAR
    };

    // Decide qué hacer tras robar la carta del mazo
    DecisionAccion decidirAccion(const Juego& juego, size_t idxPropio);

    size_t decidirRivalParaIntercambio(const Juego& juego, size_t idxPropio) const;

    // Decisiones específicas para cuando activa los efectos de las cartas 10, 11 y 12
    size_t decidirPosicionMirarPropia(const Juego& juego, size_t idxPropio) const;
    
    struct DecisionIntercambioRival {
        size_t posPropia;
        size_t idxRival;
        size_t posRival;
    };

    DecisionIntercambioRival decidirIntercambioRival(const Juego& juego, size_t idxPropio) const;

    size_t decidirPosicionDescartarPropia(const Juego& juego, size_t idxPropio) const;

    bool decidirCierre(const Juego& juego, size_t idxPropio) const;

    // marca una posición de la mano como desconocida (p.ej. tras un
    // intercambio a ciegas, donde el bot entrega y recibe sin ver nada)
    void olvidarCartaPropia(size_t pos);

    std::optional<size_t> decidirDescarteRapido(Carta cartaDescarte) const;
    void eliminarCartaPropia(size_t pos);

    // Tras intercambiar la mano, conserva conocimiento de aproximadamente
    // el 50% de las cartas nuevas y olvida el resto.
    void actualizarMemoriaTrasIntercambio(const Juego& juego, size_t idxPropio);

    // tutorial
    void establecerModoDeterminista(bool activo);
private:
    // Puntuacion estimada/valor de una carta
    int valorCarta(Carta c) const;

    // Memoria de sus propias cartas (std::nullopt indica carta desconocida)
    std::vector<std::optional<Carta>> memoriaPropia_;
    bool modoDeterminista_ = false;
};

#endif // IA_HPP_