/*
 * tipos.hpp
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

#ifndef TIPOS_HPP_
#define TIPOS_HPP_

#include <cstdint>
#include <optional>

enum class Carta : uint8_t
{
    UNO = 1,
    DOS,
    TRES,
    CUATRO,
    CINCO,
    SEIS,
    SIETE,
    OCHO,
    NUEVE,
    DIEZ,
    ONCE,
    DOCE
};

enum class TipoCartaRobada : uint8_t
{
    CARTA,
    JOKER,
    DESCARTE
};

struct CartaRobada
{
    TipoCartaRobada tipo;
    std::optional<Carta> carta;
};

enum class EstadoJuego
{
    INICIO,
    REPARTO,
    REVELADO_INICIAL,
    ESPERANDO_ROBO,
    DECIDIENDO_ACCION,
    EJECUTANDO_EFECTO,
    FIN_RONDA,
    FIN_PARTIDA
};

#endif // TIPOS_HPP_