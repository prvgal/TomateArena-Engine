/*
 * JuegoTutorial.cpp
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

#include "../include/JuegoTutorial.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>

std::vector<CartaRobada> JuegoTutorial::generarCartasNormales() const {
    auto cartas = construirCartasNormales();

    auto extraerPrimera = [&](Carta valor) -> CartaRobada {
        auto it = std::find_if(cartas.begin(), cartas.end(), [valor](const CartaRobada& c) {
            return c.tipo == TipoCartaRobada::CARTA && c.carta && c.carta.value() == valor;
        });
        if (it == cartas.end()) {
            throw std::runtime_error("JuegoTutorial::generarCartasNormales: falta carta para el reparto fijo");
        }
        CartaRobada carta = *it;
        cartas.erase(it);
        return carta;
    };

    std::vector<CartaRobada> manoJugador = {
        extraerPrimera(Carta::DOS),
        extraerPrimera(Carta::CUATRO),
        extraerPrimera(Carta::SEIS),
        extraerPrimera(Carta::TRES),
    };

    std::vector<CartaRobada> manoRival = {
        extraerPrimera(Carta::OCHO),
        extraerPrimera(Carta::OCHO),
        extraerPrimera(Carta::NUEVE),
        extraerPrimera(Carta::NUEVE),
    };

    static std::mt19937 gen(std::random_device{}());
    std::shuffle(cartas.begin(), cartas.end(), gen);

    std::vector<CartaRobada> resultado;
    resultado.reserve(manoJugador.size() + manoRival.size() + cartas.size());
    resultado.insert(resultado.end(), manoJugador.begin(), manoJugador.end());
    resultado.insert(resultado.end(), manoRival.begin(), manoRival.end());
    resultado.insert(resultado.end(), cartas.begin(), cartas.end());
    return resultado;
}

std::vector<CartaRobada> JuegoTutorial::mezclarMazoFinal(std::vector<CartaRobada> mazo) const {
    auto extraerUna = [&](auto predicado, const char* descripcion) -> CartaRobada {
        auto it = std::find_if(mazo.begin(), mazo.end(), predicado);
        if (it == mazo.end()) {
            throw std::runtime_error(
                std::string("JuegoTutorial::mezclarMazoFinal: falta en el mazo -> ") + descripcion);
        }
        CartaRobada carta = *it;
        mazo.erase(it);
        return carta;
    };

    auto esCarta = [](Carta valor) {
        return [valor](const CartaRobada& c) {
            return c.tipo == TipoCartaRobada::CARTA && c.carta && c.carta.value() == valor;
        };
    };
    auto esNumericaBaja = [](const CartaRobada& c) {
        return c.tipo == TipoCartaRobada::CARTA && c.carta &&
               c.carta.value() != Carta::DIEZ && c.carta.value() != Carta::ONCE &&
               c.carta.value() != Carta::DOCE && c.carta.value() != Carta::SIETE;
    };
    auto esTipo = [](TipoCartaRobada tipo) {
        return [tipo](const CartaRobada& c) { return c.tipo == tipo; };
    };

    std::vector<CartaRobada> ordenDeseado;
    ordenDeseado.push_back(extraerUna(esCarta(Carta::DIEZ), "un 10"));
    ordenDeseado.push_back(extraerUna(esNumericaBaja, "numérica #1 (turno IA)"));
    ordenDeseado.push_back(extraerUna(esCarta(Carta::SIETE), "un 7"));
    ordenDeseado.push_back(extraerUna(esCarta(Carta::NUEVE), "carta alta para el rival (turno IA #2)"));
    ordenDeseado.push_back(extraerUna(esTipo(TipoCartaRobada::DESCARTE), "Descarte #1 (jugador)"));
    ordenDeseado.push_back(extraerUna(esTipo(TipoCartaRobada::DESCARTE), "Descarte #2 (IA)"));
    ordenDeseado.push_back(extraerUna(esTipo(TipoCartaRobada::JOKER), "el Joker"));
    ordenDeseado.push_back(extraerUna(esCarta(Carta::ONCE), "un 11"));
    ordenDeseado.push_back(extraerUna(esCarta(Carta::DOCE), "un 12"));

    ordenDeseado.insert(ordenDeseado.end(), mazo.begin(), mazo.end());
    std::reverse(ordenDeseado.begin(), ordenDeseado.end());
    return ordenDeseado;
}