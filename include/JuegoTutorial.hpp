/*
 * JuegoTutorial.hpp
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

#ifndef JUEGO_TUTORIAL_HPP_
#define JUEGO_TUTORIAL_HPP_

#include <vector>
#include "Juego.hpp"
#include "tipos.hpp"

/// Variante de Juego usada exclusivamente por el modo Tutorial.
class JuegoTutorial : public Juego {
public:
    JuegoTutorial() = default;

protected:
    std::vector<CartaRobada> generarCartasNormales() const override;
    std::vector<CartaRobada> mezclarMazoFinal(std::vector<CartaRobada> mazo) const override;
};

#endif // JUEGO_TUTORIAL_HPP_