/*
 * Jugador.cpp
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

#include "../include/Jugador.hpp"

#include <vector>
#include <utility>
#include "../include/tipos.hpp"


void Jugador::recibirCarta(Carta carta) 
{ cartas_.push_back(carta); }

Carta Jugador::cambiarCarta(size_t posicion, Carta carta) 
{ 
    Carta saliente = cartas_.at(posicion);
    cartas_.at(posicion) = carta; 
    return saliente;
}

void Jugador::descartarCarta(size_t posicion) {
    if (posicion < cartas_.size())
        cartas_.erase(cartas_.begin() + posicion);
}

int Jugador::puntuacion() const 
{
    int punt = 0;

    for (Carta carta : cartas_)
    {
        if (carta == Carta::SIETE)
            punt -= 1;
        else 
            punt += static_cast<int>(carta);
    }

    return punt;
}

size_t Jugador::numeroCartasActuales() const 
{ return cartas_.size(); }

Carta Jugador::carta(size_t posicion) const 
{ return cartas_.at(posicion); }

void Jugador::intercambiarMano(Jugador& otro)
{ std::swap(cartas_, otro.cartas_); }

std::vector<Carta> Jugador::todasLasCartas() const
{ return cartas_; }

void Jugador::establecerCartas(const std::vector<Carta>& cartas)
{ cartas_ = cartas; }