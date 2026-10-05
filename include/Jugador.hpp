/*
 * Jugador.hpp
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

#ifndef JUGADOR_HPP_
#define JUGADOR_HPP_

#include <cstdlib>
#include <vector>

#include "../include/tipos.hpp"

class Jugador {
public:
    Jugador() = default;
    void recibirCarta(Carta carta);
    Carta cambiarCarta(size_t posicion, Carta carta);
    void descartarCarta(size_t posicion);
    int puntuacion() const;
    size_t numeroCartasActuales() const;
    Carta carta(size_t posicion) const;
    void intercambiarMano(Jugador& otro);
    std::vector<Carta> todasLasCartas() const;
    void establecerCartas(const std::vector<Carta>& cartas);
private:
    std::vector<Carta> cartas_;
};

#endif // JUGADOR_HPP_