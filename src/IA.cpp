/*
 * IA.cpp
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

#include "../include/IA.hpp"
#include <algorithm>
#include <random>

void IA::reiniciarMemoria(size_t numCartas) 
{ memoriaPropia_.assign(numCartas, std::nullopt); }

void IA::recordarCartaPropia(size_t pos, Carta carta) {
    if (pos < memoriaPropia_.size())
        memoriaPropia_[pos] = carta;
}

IA::DecisionAccion IA::decidirAccion(const Juego& juego, size_t idxPropio) {
    auto optCartaRobada = juego.cartaRobada();

    if (!optCartaRobada)
        return {AccionTurno::DESAGRADO_DESCARTAR, 0};
    if (optCartaRobada->tipo == TipoCartaRobada::JOKER)
        return {AccionTurno::USAR_EFECTO, 0};
    if (optCartaRobada->tipo == TipoCartaRobada::DESCARTE)
        return {AccionTurno::USAR_EFECTO, 0};
    if (!optCartaRobada->carta)
        return {AccionTurno::DESAGRADO_DESCARTAR, 0};

    Carta robada = optCartaRobada->carta.value();

    int valRobada = valorCarta(robada);

    size_t numCartasMano =
        juego.jugador(idxPropio).numeroCartasActuales();

    // Asegurar que la memoria tiene el mismo tamaño que la mano
    if (memoriaPropia_.size() != numCartasMano)
        memoriaPropia_.resize(numCartasMano, std::nullopt);

    // Si la carta es de valor muy bajo (-1 o 1 a 4), nos interesa intercambiarla
    // Buscamos en la memoria la peor carta conocida o una desconocida
    size_t mejorPosReemplazo = 0;
    int maxValorMano = -99;
    bool cartaEncontrada = false;

    for (size_t i = 0; i < numCartasMano; ++i) 
    {
        if (memoriaPropia_[i].has_value()) 
        {
            int valMano = valorCarta(memoriaPropia_[i].value());
        
            if (valMano > maxValorMano) 
            {
                maxValorMano = valMano;
                mejorPosReemplazo = i;
                cartaEncontrada = true;
            }
        }
    }

    // Si la carta robada es mejor que la peor carta que recordamos en la mano
    if (cartaEncontrada && valRobada < maxValorMano) 
    {
        memoriaPropia_[mejorPosReemplazo] = robada; // Actualizar memoria con la nueva carta
        return {AccionTurno::INTERCAMBIAR, mejorPosReemplazo};
    }

    // Si tenemos alguna carta desconocida en la mano y la carta robada es aceptable (<= 4)
    if (valRobada <= 4) 
    {
        for (size_t i = 0; i < numCartasMano; ++i) 
        {
            if (!memoriaPropia_[i].has_value()) 
            {
                memoriaPropia_[i] = robada;
                return {AccionTurno::INTERCAMBIAR, i};
            }
        }
    }

    // Si la carta es un 10, 11 o 12 y no la usamos para la mano, preferimos USAR_EFECTO
    if (robada == Carta::DIEZ || robada == Carta::ONCE || 
        robada == Carta::DOCE)
        return {AccionTurno::USAR_EFECTO, 0};

    // Si no conviene intercambiar ni es/quiere usar un efecto, se descarta
    return {AccionTurno::DESAGRADO_DESCARTAR, 0};
}

void IA::establecerModoDeterminista(bool activo) { modoDeterminista_ = activo; }

size_t IA::decidirRivalParaIntercambio(const Juego& juego, size_t idxPropio) const {
    size_t totalJugadores = juego.numeroJugadores();
    if (totalJugadores <= 1) return idxPropio;

    std::vector<size_t> rivales;
    for (size_t i = 0; i < totalJugadores; ++i)
        if (i != idxPropio) rivales.push_back(i);

    if (modoDeterminista_) return rivales.front();

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, rivales.size() - 1);
    return rivales[dist(gen)];
}

size_t IA::decidirPosicionMirarPropia(const Juego& juego, size_t idxPropio) const {
    size_t numCartasMano = juego.jugador(idxPropio).numeroCartasActuales();

    // prioridad: mirar una carta de la mano que sea desconocida
    for (size_t i = 0; i < numCartasMano; ++i)
        if (i < memoriaPropia_.size() && !memoriaPropia_[i].has_value())
            return i;
    
    return 0; // Si conoce todas, mira la primera por defecto
}

IA::DecisionIntercambioRival IA::decidirIntercambioRival(const Juego& juego, size_t idxPropio) const {
    size_t numCartasMano = juego.jugador(idxPropio).numeroCartasActuales();

    // buscar nuestra peor carta conocida para entregarla al rival
    size_t peorPosPropia = 0;
    int maxValor = -99;

    for (size_t i = 0; i < numCartasMano; ++i) 
    {
        if (i < memoriaPropia_.size() && memoriaPropia_[i].has_value()) 
        {
            int val = valorCarta(memoriaPropia_[i].value());

            if (val > maxValor) 
            {
                maxValor = val;
                peorPosPropia = i;
            }
        }
    }

    size_t idxRival = decidirRivalParaIntercambio(juego, idxPropio);
    size_t posRival = 0; // se sigue asumiendo posición 0 a ciegas

    return {peorPosPropia, idxRival, posRival};
}

size_t IA::decidirPosicionDescartarPropia(const Juego& juego, size_t idxPropio) const {
    size_t numCartasMano = juego.jugador(idxPropio).numeroCartasActuales();

    size_t mejorPos = 0;
    int maxValor = -99;
    bool encontradaPositiva = false;

    for (size_t i = 0; i < numCartasMano; ++i)
    {
        if (i < memoriaPropia_.size() && memoriaPropia_[i].has_value())
        {
            int val = valorCarta(memoriaPropia_[i].value());

            // Solo candidatas las cartas con valor positivo: un 7 conocido
            // (-1) conviene conservarlo, no descartarlo
            if (val > 0 && val > maxValor)
            {
                maxValor = val;
                mejorPos = i;
                encontradaPositiva = true;
            }
        }
    }

    if (encontradaPositiva)
        return mejorPos;

    std::vector<size_t> desconocidas;
    for (size_t i = 0; i < numCartasMano; ++i)
        if (i >= memoriaPropia_.size() || !memoriaPropia_[i].has_value())
            desconocidas.push_back(i);

    if (!desconocidas.empty()) {
        if (modoDeterminista_) return desconocidas.front();
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, desconocidas.size() - 1);
        return desconocidas[dist(gen)];
    }

    return 0;
}

bool IA::decidirCierre(const Juego& juego, size_t idxPropio) const {
    size_t numCartasMano = juego.jugador(idxPropio).numeroCartasActuales();
    if (numCartasMano == 0) return false;

    int sumaConocida = 0;
    size_t conocidas = 0;

    for (size_t i = 0; i < numCartasMano; ++i) {
        if (i < memoriaPropia_.size() && memoriaPropia_[i].has_value()) {
            sumaConocida += valorCarta(memoriaPropia_[i].value());
            ++conocidas;
        }
    }

    size_t desconocidas = numCartasMano - conocidas;
    double estimacion = sumaConocida + static_cast<double>(desconocidas) * 5.5;

    bool conoceMayoria = conocidas * 2 >= numCartasMano;
    return conoceMayoria && estimacion <= 4.0;
}

void IA::olvidarCartaPropia(size_t pos) {
    if (pos < memoriaPropia_.size())
        memoriaPropia_[pos] = std::nullopt;
}

std::optional<size_t> IA::decidirDescarteRapido(Carta cartaDescarte) const {
    if (cartaDescarte == Carta::SIETE)
        return std::nullopt;
    
    for (size_t i = 0; i < memoriaPropia_.size(); ++i) {
        if (memoriaPropia_[i].has_value() && memoriaPropia_[i].value() == cartaDescarte)
            return i;
    }
    
    return std::nullopt;
}

void IA::eliminarCartaPropia(size_t pos) {
    if (pos < memoriaPropia_.size())
        memoriaPropia_.erase(memoriaPropia_.begin() + pos); // desplaza, no deja hueco
}

int IA::valorCarta(Carta c) const {
    if (c == Carta::SIETE) return -1;
    else  return static_cast<int>(c);
}

void IA::actualizarMemoriaTrasIntercambio(const Juego& juego, size_t idxPropio) {
    size_t numCartas = juego.jugador(idxPropio).numeroCartasActuales();
    memoriaPropia_.assign(numCartas, std::nullopt);
    size_t cantidadConocida = numCartas / 2;
    if (cantidadConocida == 0) return;

    std::vector<size_t> posiciones(numCartas);
    for (size_t i = 0; i < numCartas; ++i) posiciones[i] = i;

    if (!modoDeterminista_) {
        static std::mt19937 gen(std::random_device{}());
        std::shuffle(posiciones.begin(), posiciones.end(), gen);
    }

    for (size_t i = 0; i < cantidadConocida; ++i) {
        size_t pos = posiciones[i];
        memoriaPropia_[pos] = juego.jugador(idxPropio).carta(pos);
    }
}