/*
 * Juego.cpp
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

#include "../include/Juego.hpp"

#include <algorithm>
#include <random>
#include <vector>
#include <sstream>
#include <cstdlib>

#include "../include/tipos.hpp"

namespace {
    constexpr const char* VERSION_SERIALIZACION = "TOMATE_SAVE_V2";

    std::string unirConComas(const std::vector<std::string>& tokens) {
        std::string resultado;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) resultado += ',';
            resultado += tokens[i];
        }
        return resultado;
    }

    std::vector<std::string> dividirPorComas(const std::string& texto) {
        std::vector<std::string> tokens;
        if (texto.empty()) return tokens;
        std::stringstream ss(texto);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        return tokens;
    }
}

Juego::Juego() : jugadorActual_{0}, estado_{EstadoJuego::INICIO} {}

void Juego::iniciar(size_t numJugadores) {
    // crear mazo y barajar
    std::vector<CartaRobada> cartasNormales = generarCartasNormales();
    
    // crear jugadores
    jugadores_.clear();
    jugadores_.resize(numJugadores);

    estado_ = EstadoJuego::REPARTO;

    size_t indiceCarta = 0;

    for (Jugador& jugador : jugadores_)
        for (size_t j = 0; j < CARTAS_MANO; ++j)
        {
            jugador.recibirCarta(cartasNormales[indiceCarta].carta.value());
            ++indiceCarta;
        }

    // vaciar monton_ y descarte_ por si iniciar() se llama más de una vez
    monton_ = std::stack<CartaRobada>{};
    descarte_ = std::stack<CartaRobada>{};

    puntuacionesIniciales_.assign(numJugadores, 0);
    for (size_t i = 0; i < numJugadores; ++i)
        puntuacionesIniciales_[i] = jugadores_[i].puntuacion();

    cartasEspecialesUsadas_.assign(numJugadores, 0);
    jugadorQueCerro_.reset();
    ultimoCriterioDesempate_ = 0;

    for (; indiceCarta < cartasNormales.size(); ++indiceCarta)
        monton_.push(cartasNormales[indiceCarta]);

    for (size_t i = 0; i < NUM_JOKERS; ++i)
        monton_.push({TipoCartaRobada::JOKER, std::nullopt});
    for (size_t i = 0; i < NUM_DESCARTES; ++i)
        monton_.push({TipoCartaRobada::DESCARTE, std::nullopt});
    

    std::vector<CartaRobada> cartasMazo;

    while (!monton_.empty())
    {
        cartasMazo.push_back(monton_.top());
        monton_.pop();
    }

    cartasMazo = mezclarMazoFinal(std::move(cartasMazo));

    for (const CartaRobada& carta : cartasMazo)
        monton_.push(carta);

    ganadorPorCartas_.reset();
    jugadorActual_ = 0;
    estado_ = EstadoJuego::REVELADO_INICIAL;
    cierre_ = false;
    turnosRestantes_ = 0;
    cartaRobada_.reset();
    ultimoJugadorQueActuo_ = static_cast<size_t>(-1);
    huboRebarajeUltimoRobo_ = false;
}

std::optional<Carta> Juego::consultarCartaMano(size_t idxJugador, size_t posMano) const {
    if (idxJugador >= jugadores_.size()) return std::nullopt;
    if (posMano >= jugadores_[idxJugador].numeroCartasActuales()) return std::nullopt;

    return jugadores_[idxJugador].carta(posMano);
}

void Juego::terminarFaseRevelado() {
    if (estado_ == EstadoJuego::REVELADO_INICIAL)
        estado_ = EstadoJuego::ESPERANDO_ROBO;
}

std::vector<int> Juego::obtenerPuntuaciones() const {
    std::vector<int> puntos;
    puntos.reserve(jugadores_.size());
    
    for (const auto& jug : jugadores_) 
        puntos.push_back(jug.puntuacion());
    
    return puntos;
}

std::optional<size_t> Juego::obtenerGanador(bool soloHumanoDesempata) const {
    ultimoCriterioDesempate_ = 0;

    if (estado_ != EstadoJuego::FIN_PARTIDA || jugadores_.empty())
        return std::nullopt;
    if (ganadorPorCartas_)
        return ganadorPorCartas_; // se quedó sin cartas: gana directo, no hay empate posible

    int minPuntos = jugadores_[0].puntuacion();
    for (size_t i = 1; i < jugadores_.size(); ++i)
        minPuntos = std::min(minPuntos, jugadores_[i].puntuacion());

    std::vector<size_t> empatados;
    for (size_t i = 0; i < jugadores_.size(); ++i)
        if (jugadores_[i].puntuacion() == minPuntos) empatados.push_back(i);

    if (empatados.size() == 1) return empatados.front();

    // --- CRITEIRO EMPATE ---

    // Criterio 1: si quien declaró el cierre está entre los empatados, gana él.
    if (jugadorQueCerro_ &&
        std::find(empatados.begin(), empatados.end(), *jugadorQueCerro_) != empatados.end())
    {
        ultimoCriterioDesempate_ = 1;
        return *jugadorQueCerro_;
    }

    if (soloHumanoDesempata) {
        // Modo local: solo se aplica el cierre
        ultimoCriterioDesempate_ = 5;
        auto it = std::find(empatados.begin(), empatados.end(), size_t{0});
        return it != empatados.end() ? 0 : empatados.front();
    }

    // Criterio 2: menos cartas en mano
    size_t minCartas = jugadores_[empatados.front()].numeroCartasActuales();
    for (size_t idx : empatados) minCartas = std::min(minCartas, jugadores_[idx].numeroCartasActuales());
    std::vector<size_t> tras2;
    for (size_t idx : empatados)
        if (jugadores_[idx].numeroCartasActuales() == minCartas) tras2.push_back(idx);
    if (tras2.size() == 1) { ultimoCriterioDesempate_ = 2; return tras2.front(); }

    // Criterio 3: menos cartas especiales usadas
    size_t minEspeciales = cartasEspecialesUsadas_[tras2.front()];
    for (size_t idx : tras2) minEspeciales = std::min(minEspeciales, cartasEspecialesUsadas_[idx]);
    std::vector<size_t> tras3;
    for (size_t idx : tras2)
        if (cartasEspecialesUsadas_[idx] == minEspeciales) tras3.push_back(idx);
    if (tras3.size() == 1) { ultimoCriterioDesempate_ = 3; return tras3.front(); }

    // Criterio 4: mejor progresión (mayor mejora respecto a la puntuación inicial)
    int mejorProgreso = puntuacionesIniciales_[tras3.front()] - jugadores_[tras3.front()].puntuacion();
    size_t ganador4 = tras3.front();
    for (size_t idx : tras3) {
        int progreso = puntuacionesIniciales_[idx] - jugadores_[idx].puntuacion();
        if (progreso > mejorProgreso) { mejorProgreso = progreso; ganador4 = idx; }
    }
    ultimoCriterioDesempate_ = 4;
    return ganador4;
}

int Juego::ultimoCriterioDesempate() const { return ultimoCriterioDesempate_; }

bool Juego::robarDelMonton() {
    if (estado_ != EstadoJuego::ESPERANDO_ROBO)
        return false;

    huboRebarajeUltimoRobo_ = false;

    if (monton_.empty())
        reponerMontonDesdeDescarte();

    if (monton_.empty())
        return false; // no queda nada ni en el montón ni en el descarte: fin real de partida

    cartaRobada_ = monton_.top(); 
    monton_.pop(); 

    estado_ = EstadoJuego::DECIDIENDO_ACCION;
    return true;
}

bool Juego::huboRebarajeUltimoRobo() const {
    return huboRebarajeUltimoRobo_;
}

bool Juego::cerrarPartida(size_t idxJugador) {
    if (idxJugador >= jugadores_.size()) return false;
    if (cierre_ || estado_ == EstadoJuego::FIN_PARTIDA) return false;
    if (idxJugador != jugadorActual_ && idxJugador != ultimoJugadorQueActuo_) return false;

    cierre_ = true;
    jugadorQueCerro_ = idxJugador;

    if (idxJugador == jugadorActual_)
        turnosRestantes_ = jugadores_.size() - 1;
    else
        turnosRestantes_ = jugadores_.size() - 2;

    return true;
}

bool Juego::penalizarJugador(size_t idxJugador)
{
    if (idxJugador >= jugadores_.size())
        return false;

    constexpr size_t CANTIDAD_PENALIZACION = 2;
    size_t robadas = 0;

    std::vector<CartaRobada> especialesApartadas;

    while (robadas < CANTIDAD_PENALIZACION && !descarte_.empty())
    {
        CartaRobada carta = descarte_.top();
        descarte_.pop();

        if (carta.tipo == TipoCartaRobada::CARTA && carta.carta)
        {
            jugadores_[idxJugador].recibirCarta(carta.carta.value());
            ++robadas;
        }
        else
        {
            especialesApartadas.push_back(carta);
        }
    }

    for (auto it = especialesApartadas.rbegin(); it != especialesApartadas.rend(); ++it)
        descarte_.push(*it);

    while (robadas < CANTIDAD_PENALIZACION && !monton_.empty())
    {
        CartaRobada carta = monton_.top();
        monton_.pop();

        if (carta.tipo == TipoCartaRobada::CARTA && carta.carta)
        {
            jugadores_[idxJugador].recibirCarta(carta.carta.value());
            ++robadas;
        }
        else
            descarte_.push(carta);
    }

    if (robadas < CANTIDAD_PENALIZACION)
    {
        reponerMontonDesdeDescarte();

        while (robadas < CANTIDAD_PENALIZACION && !monton_.empty())
        {
            CartaRobada carta = monton_.top();
            monton_.pop();

            if (carta.tipo == TipoCartaRobada::CARTA && carta.carta)
            {
                jugadores_[idxJugador].recibirCarta(carta.carta.value());
                ++robadas;
            }
            else
                descarte_.push(carta);
        }
    }

    if (monton_.empty() && descarte_.size() <= 1)
        estado_ = EstadoJuego::FIN_PARTIDA;

    return robadas > 0;
}

bool Juego::descartarCartaRobada() {
    if (estado_ != EstadoJuego::DECIDIENDO_ACCION || !cartaRobada_) 
        return false;

    descarte_.push(cartaRobada_.value());
    cartaRobada_.reset();

    avanzarTurno();
    
    return true;
}

bool Juego::intercambiarCartaRobada(size_t posMano) {
    if (estado_ != EstadoJuego::DECIDIENDO_ACCION || !cartaRobada_) 
        return false;
    if (cartaRobada_->tipo != TipoCartaRobada::CARTA || !cartaRobada_->carta)
        return false;
    if (posMano >= jugadores_[jugadorActual_].numeroCartasActuales())
        return false;

    Carta saliente = jugadores_[jugadorActual_].cambiarCarta(posMano, cartaRobada_->carta.value());
    descarte_.push({TipoCartaRobada::CARTA, saliente});
    cartaRobada_.reset();
    
    avanzarTurno();
    return true;
}

bool Juego::intentarDescarteFueraDeTurno(size_t idxJugador, size_t posMano) {
    huboPenalizacionUltimoIntento_ = false;

    if (descarte_.empty()) return false;
    if (idxJugador >= jugadores_.size()) return false;
    if (posMano >= jugadores_[idxJugador].numeroCartasActuales()) return false;

    const CartaRobada& cartaTop = descarte_.top();

    if (cartaTop.tipo != TipoCartaRobada::CARTA || !cartaTop.carta)
        return false;

    Carta cartaJugador = jugadores_[idxJugador].carta(posMano);
    Carta cartaDescarte = cartaTop.carta.value();

    if (cartaJugador == cartaDescarte)
    {
        jugadores_[idxJugador].descartarCarta(posMano);

        descarte_.push({TipoCartaRobada::CARTA, cartaJugador});

        if (jugadores_[idxJugador].numeroCartasActuales() == 0)
        {
            ganadorPorCartas_ = idxJugador;
            estado_ = EstadoJuego::FIN_PARTIDA;
        }

        return true;
    }

    // Se equivocó penaliza salvo que ya tenga demasiadas cartas en mano
    if (jugadores_[idxJugador].numeroCartasActuales() > UMBRAL_CARTAS_SIN_PENALIZACION)
        return false;

    huboPenalizacionUltimoIntento_ = true;
    penalizarJugador(idxJugador);
    return false;
}

void Juego::forzarJugadorActual(size_t idx) {
    if (idx < jugadores_.size()) {
        jugadorActual_ = idx;
    }
}

void Juego::ejecutarSorteoOrdenInicial() {
    ultimoSorteoOrdenInicial_.assign(jugadores_.size(), -99);
    std::vector<CartaRobada> extraidas;

    for (size_t i = 0; i < jugadores_.size(); ++i) {
        while (!monton_.empty()) {
            CartaRobada c = monton_.top();
            monton_.pop();
            extraidas.push_back(c);

            if (c.tipo == TipoCartaRobada::CARTA && c.carta) {
                ultimoSorteoOrdenInicial_[i] =
                    (*c.carta == Carta::SIETE) ? -1 : static_cast<int>(*c.carta);
                break;
            }
        }
    }

    for (auto it = extraidas.rbegin(); it != extraidas.rend(); ++it)
        monton_.push(*it);
}

int Juego::valorSorteoOrdenInicial(size_t idxJugador) const {
    if (idxJugador >= ultimoSorteoOrdenInicial_.size()) return -99;
    return ultimoSorteoOrdenInicial_[idxJugador];
}

bool Juego::usarEfectoCartaRobada() {
    if (estado_ != EstadoJuego::DECIDIENDO_ACCION || !cartaRobada_) 
        return false;
    if (cartaRobada_->tipo == TipoCartaRobada::JOKER || 
            cartaRobada_->tipo == TipoCartaRobada::DESCARTE)
    {
        descarte_.push(cartaRobada_.value());
        cartaRobada_.reset();
        estado_ = EstadoJuego::EJECUTANDO_EFECTO;
        if (jugadorActual_ < cartasEspecialesUsadas_.size()) ++cartasEspecialesUsadas_[jugadorActual_];
        return true;
    }
    if (!cartaRobada_->carta)
        return false;

     Carta c = cartaRobada_->carta.value();

    if (c == Carta::DIEZ || c == Carta::ONCE || c == Carta::DOCE)
    {
        descarte_.push(cartaRobada_.value());
        cartaRobada_.reset();
        estado_ = EstadoJuego::EJECUTANDO_EFECTO;
        if (jugadorActual_ < cartasEspecialesUsadas_.size()) ++cartasEspecialesUsadas_[jugadorActual_];
        return true;
    }

    return false;
}

std::optional<Carta> Juego::efectoMirarCarta(size_t posMano) {
    if (estado_ != EstadoJuego::EJECUTANDO_EFECTO) return std::nullopt;
    if (posMano >= jugadores_[jugadorActual_].numeroCartasActuales()) return std::nullopt;

    Carta c = jugadores_[jugadorActual_].carta(posMano);
    avanzarTurno();

    return c;
}

bool Juego::efectoIntercambiarCiegas(size_t posPropia, size_t idxRival, size_t posRival) {
    if (estado_ != EstadoJuego::EJECUTANDO_EFECTO) return false;
    if (idxRival >= jugadores_.size() || idxRival == jugadorActual_) return false;
    if (posPropia >= jugadores_[jugadorActual_].numeroCartasActuales()) return false;
    if (posRival >= jugadores_[idxRival].numeroCartasActuales()) return false;

    Carta miCarta = jugadores_[jugadorActual_].carta(posPropia);
    Carta cartaRival = jugadores_[idxRival].carta(posRival);

    jugadores_[jugadorActual_].cambiarCarta(posPropia, cartaRival);
    jugadores_[idxRival].cambiarCarta(posRival, miCarta);

    avanzarTurno();
    return true;
}

std::optional<Carta> Juego::efectoIntercambiarMirando(size_t posMano, size_t idxRival, size_t posRival) {
    if (estado_ != EstadoJuego::EJECUTANDO_EFECTO) return std::nullopt;
    if (idxRival >= jugadores_.size() || idxRival == jugadorActual_) return std::nullopt;
    if (posMano >= jugadores_[jugadorActual_].numeroCartasActuales()) return std::nullopt;
    if (posRival >= jugadores_[idxRival].numeroCartasActuales()) return std::nullopt;

    Carta miCarta = jugadores_[jugadorActual_].carta(posMano);
    Carta cartaRival = jugadores_[idxRival].carta(posRival);

    jugadores_[jugadorActual_].cambiarCarta(posMano, cartaRival);
    jugadores_[idxRival].cambiarCarta(posRival, miCarta);

    avanzarTurno();
    return cartaRival;
}

bool Juego::efectoIntercambiarManos(size_t idx1, size_t idx2) 
{
    if (estado_ != EstadoJuego::EJECUTANDO_EFECTO)
        return false;
    if (idx1 >= jugadores_.size() || idx2 >= jugadores_.size())
        return false;
    if (idx1 == idx2)
        return false;
    if (idx1 != jugadorActual_ && idx2 != jugadorActual_)
        return false;

    jugadores_[idx1].intercambiarMano(jugadores_[idx2]);
    avanzarTurno();

    return true;
}

bool Juego::efectoDescartarPropia(size_t posMano) {
    if (estado_ != EstadoJuego::EJECUTANDO_EFECTO) return false;
    if (posMano >= jugadores_[jugadorActual_].numeroCartasActuales()) return false;

    Carta carta = jugadores_[jugadorActual_].carta(posMano);
    jugadores_[jugadorActual_].descartarCarta(posMano);
    descarte_.push({TipoCartaRobada::CARTA, carta});

    if (jugadores_[jugadorActual_].numeroCartasActuales() == 0)
    {
        ganadorPorCartas_ = jugadorActual_;
        estado_ = EstadoJuego::FIN_PARTIDA;
        return true;
    }

    avanzarTurno();
    return true;
}

/* ------------------------------------------------------------ */
/* Serialización de estado                                       */
/* ------------------------------------------------------------ */

std::string Juego::codificarCartaRobada(const CartaRobada& c) {
    switch (c.tipo) {
        case TipoCartaRobada::JOKER: return "J";
        case TipoCartaRobada::DESCARTE: return "D";
        case TipoCartaRobada::CARTA:
            return "C" + std::to_string(c.carta ? static_cast<int>(*c.carta) : 0);
    }
    return "?";
}

bool Juego::decodificarCartaRobada(const std::string& token, CartaRobada& out) {
    if (token.empty()) return false;
    if (token == "J") { out = {TipoCartaRobada::JOKER, std::nullopt}; return true; }
    if (token == "D") { out = {TipoCartaRobada::DESCARTE, std::nullopt}; return true; }
    if (token[0] == 'C') {
        if (token.size() < 2) return false;
        char* fin = nullptr;
        long valor = std::strtol(token.c_str() + 1, &fin, 10);
        if (fin == token.c_str() + 1 || *fin != '\0') return false; // no se consumió todo el resto
        if (valor < 1 || valor > 12) return false;
        out = {TipoCartaRobada::CARTA, static_cast<Carta>(valor)};
        return true;
    }
    return false;
}

std::vector<CartaRobada> Juego::obtenerPilaComoVector(const std::stack<CartaRobada>& pila) const {
    std::stack<CartaRobada> copia = pila; // copia completa, no consume la original
    std::vector<CartaRobada> resultado;
    resultado.reserve(copia.size());
    while (!copia.empty()) {
        resultado.push_back(copia.top());
        copia.pop();
    }
    return resultado; // orden cima primero, fondo al final
}

std::stack<CartaRobada> Juego::construirPilaDesdeVector(const std::vector<CartaRobada>& vec) {
    std::stack<CartaRobada> pila;
    for (auto it = vec.rbegin(); it != vec.rend(); ++it)
        pila.push(*it);
    return pila;
}

std::string Juego::serializarEstado() const {
    std::ostringstream out;
    out << VERSION_SERIALIZACION << "\n";
    out << jugadores_.size() << "\n";
    out << jugadorActual_ << "\n";
    out << static_cast<int>(estado_) << "\n";
    out << (cierre_ ? 1 : 0) << "\n";
    out << turnosRestantes_ << "\n";
    out << (ganadorPorCartas_ ? static_cast<long long>(*ganadorPorCartas_) : -1LL) << "\n";
    out << (ultimoJugadorQueActuo_ == static_cast<size_t>(-1)
                ? -1LL
                : static_cast<long long>(ultimoJugadorQueActuo_)) << "\n";
    out << (huboRebarajeUltimoRobo_ ? 1 : 0) << "\n";

    if (cartaRobada_) out << codificarCartaRobada(*cartaRobada_) << "\n";
    else out << "-" << "\n";

    {
        std::vector<std::string> tokens;
        for (const auto& c : obtenerPilaComoVector(monton_)) tokens.push_back(codificarCartaRobada(c));
        out << unirConComas(tokens) << "\n";
    }
    {
        std::vector<std::string> tokens;
        for (const auto& c : obtenerPilaComoVector(descarte_)) tokens.push_back(codificarCartaRobada(c));
        out << unirConComas(tokens) << "\n";
    }
    {
        std::vector<std::string> tokens;
        for (int v : ultimoSorteoOrdenInicial_) tokens.push_back(std::to_string(v));
        out << unirConComas(tokens) << "\n";
    }

    out << (jugadorQueCerro_ ? static_cast<long long>(*jugadorQueCerro_) : -1LL) << "\n";
    {
        std::vector<std::string> tokens;
        for (int v : puntuacionesIniciales_) tokens.push_back(std::to_string(v));
        out << unirConComas(tokens) << "\n";
    }
    {
        std::vector<std::string> tokens;
        for (size_t v : cartasEspecialesUsadas_) tokens.push_back(std::to_string(v));
        out << unirConComas(tokens) << "\n";
    }

    for (const auto& jugador : jugadores_) {
        std::vector<std::string> tokens;
        for (Carta c : jugador.todasLasCartas()) tokens.push_back(std::to_string(static_cast<int>(c)));
        out << unirConComas(tokens) << "\n";
    }

    return out.str();
}

bool Juego::restaurarEstado(const std::string& datos) {
    std::istringstream in(datos);
    std::string linea;

    auto leerLinea = [&](std::string& destino) -> bool {
        if (!std::getline(in, linea)) return false;
        destino = linea;
        return true;
    };

    auto leerEntero = [&](long long& destino) -> bool {
        std::string l;
        if (!leerLinea(l)) return false;
        if (l.empty()) return false;
        char* fin = nullptr;
        destino = std::strtoll(l.c_str(), &fin, 10);
        return fin == l.c_str() + l.size();
    };

    std::string version;
    if (!leerLinea(version) || version != VERSION_SERIALIZACION) return false;

    long long numJugadoresLL;
    if (!leerEntero(numJugadoresLL) || numJugadoresLL <= 0) return false;
    size_t numJugadores = static_cast<size_t>(numJugadoresLL);

    long long jugadorActualLL;
    if (!leerEntero(jugadorActualLL) || jugadorActualLL < 0 ||
        static_cast<size_t>(jugadorActualLL) >= numJugadores) return false;

    long long estadoLL;
    if (!leerEntero(estadoLL) || estadoLL < 0 || estadoLL > 7) return false; // 0..7 = rango de EstadoJuego

    long long cierreLL;
    if (!leerEntero(cierreLL)) return false;

    long long turnosRestantesLL;
    if (!leerEntero(turnosRestantesLL) || turnosRestantesLL < 0) return false;

    long long ganadorLL;
    if (!leerEntero(ganadorLL)) return false;
    if (ganadorLL >= static_cast<long long>(numJugadores)) return false;

    long long ultimoJugadorLL;
    if (!leerEntero(ultimoJugadorLL)) return false;
    if (ultimoJugadorLL >= static_cast<long long>(numJugadores)) return false;

    long long huboRebarajeLL;
    if (!leerEntero(huboRebarajeLL)) return false;

    std::string cartaRobadaTok;
    if (!leerLinea(cartaRobadaTok)) return false;
    std::optional<CartaRobada> cartaRobadaNueva;
    if (cartaRobadaTok != "-") {
        CartaRobada cr;
        if (!decodificarCartaRobada(cartaRobadaTok, cr)) return false;
        cartaRobadaNueva = cr;
    }

    std::string montonTok, descarteTok, sorteoTok;
    if (!leerLinea(montonTok)) return false;
    if (!leerLinea(descarteTok)) return false;
    if (!leerLinea(sorteoTok)) return false;

    std::vector<CartaRobada> montonNuevo, descarteNuevo;
    for (const auto& tok : dividirPorComas(montonTok)) {
        CartaRobada c;
        if (!decodificarCartaRobada(tok, c)) return false;
        montonNuevo.push_back(c);
    }
    for (const auto& tok : dividirPorComas(descarteTok)) {
        CartaRobada c;
        if (!decodificarCartaRobada(tok, c)) return false;
        descarteNuevo.push_back(c);
    }

    std::vector<int> sorteoNuevo;
    for (const auto& tok : dividirPorComas(sorteoTok)) {
        char* fin = nullptr;
        long v = std::strtol(tok.c_str(), &fin, 10);
        if (fin != tok.c_str() + tok.size()) return false;
        sorteoNuevo.push_back(static_cast<int>(v));
    }
    if (!sorteoNuevo.empty() && sorteoNuevo.size() != numJugadores) return false;

    long long jugadorQueCerroLL;
    if (!leerEntero(jugadorQueCerroLL)) return false;
    if (jugadorQueCerroLL >= static_cast<long long>(numJugadores)) return false;

    std::string puntIniTok, especialesTok;
    if (!leerLinea(puntIniTok)) return false;
    if (!leerLinea(especialesTok)) return false;

    std::vector<int> puntuacionesIniNuevo;
    for (const auto& tok : dividirPorComas(puntIniTok)) {
        char* fin = nullptr;
        long v = std::strtol(tok.c_str(), &fin, 10);
        if (fin != tok.c_str() + tok.size()) return false;
        puntuacionesIniNuevo.push_back(static_cast<int>(v));
    }
    if (!puntuacionesIniNuevo.empty() && puntuacionesIniNuevo.size() != numJugadores) return false;

    std::vector<size_t> especialesNuevo;
    for (const auto& tok : dividirPorComas(especialesTok)) {
        char* fin = nullptr;
        long v = std::strtol(tok.c_str(), &fin, 10);
        if (fin != tok.c_str() + tok.size() || v < 0) return false;
        especialesNuevo.push_back(static_cast<size_t>(v));
    }
    if (!especialesNuevo.empty() && especialesNuevo.size() != numJugadores) return false;

    std::vector<std::vector<Carta>> manosNuevas;
    manosNuevas.reserve(numJugadores);
    for (size_t i = 0; i < numJugadores; ++i) {
        std::string manoTok;
        if (!leerLinea(manoTok)) return false;
        std::vector<Carta> mano;
        for (const auto& tok : dividirPorComas(manoTok)) {
            char* fin = nullptr;
            long v = std::strtol(tok.c_str(), &fin, 10);
            if (fin != tok.c_str() + tok.size() || v < 1 || v > 12) return false;
            mano.push_back(static_cast<Carta>(v));
        }
        manosNuevas.push_back(std::move(mano));
    }

    jugadores_.clear();
    jugadores_.resize(numJugadores);
    for (size_t i = 0; i < numJugadores; ++i)
        jugadores_[i].establecerCartas(manosNuevas[i]);

    monton_ = construirPilaDesdeVector(montonNuevo);
    descarte_ = construirPilaDesdeVector(descarteNuevo);

    jugadorActual_ = static_cast<size_t>(jugadorActualLL);
    estado_ = static_cast<EstadoJuego>(estadoLL);
    cartaRobada_ = cartaRobadaNueva;
    turnosRestantes_ = static_cast<size_t>(turnosRestantesLL);
    cierre_ = cierreLL != 0;
    ganadorPorCartas_ = (ganadorLL < 0) ? std::nullopt : std::optional<size_t>(static_cast<size_t>(ganadorLL));
    ultimoJugadorQueActuo_ = (ultimoJugadorLL < 0) ? static_cast<size_t>(-1) : static_cast<size_t>(ultimoJugadorLL);
    huboRebarajeUltimoRobo_ = huboRebarajeLL != 0;
    ultimoSorteoOrdenInicial_ = sorteoNuevo;

    jugadorQueCerro_ = (jugadorQueCerroLL < 0) ? std::nullopt : std::optional<size_t>(static_cast<size_t>(jugadorQueCerroLL));
    puntuacionesIniciales_ = puntuacionesIniNuevo;
    cartasEspecialesUsadas_ = especialesNuevo;
    ultimoCriterioDesempate_ = 0;

    return true;
}

/* Miembros protegidos */
std::vector<CartaRobada> Juego::construirCartasNormales() const {
    std::vector<CartaRobada> cartasNormales;
    cartasNormales.reserve(NUM_PALOS * NUM_VALORES);

    for (size_t palo = 0; palo < NUM_PALOS; ++palo)
        for (size_t valor = 1; valor <= NUM_VALORES; ++valor)
            cartasNormales.push_back({TipoCartaRobada::CARTA, static_cast<Carta>(valor)});

    return cartasNormales;
}

std::vector<CartaRobada> Juego::generarCartasNormales() const {
    auto cartasNormales = construirCartasNormales();
    static std::mt19937 gen(std::random_device{}());
    std::shuffle(cartasNormales.begin(), cartasNormales.end(), gen);
    return cartasNormales;
}

std::vector<CartaRobada> Juego::mezclarMazoFinal(std::vector<CartaRobada> mazo) const {
    static std::mt19937 gen(std::random_device{}());
    std::shuffle(mazo.begin(), mazo.end(), gen);
    return mazo;
}

/* Miembros privados */
void Juego::avanzarTurno() {
    ultimoJugadorQueActuo_ = jugadorActual_;
    jugadorActual_ = (jugadorActual_+1) % jugadores_.size();

    if (cierre_) 
    {
        if (turnosRestantes_ == 0) 
        {        
            estado_ = EstadoJuego::FIN_PARTIDA;
            return;
        }

        --turnosRestantes_;
    }

    if (monton_.empty() && descarte_.size() <= 1)
        estado_ = EstadoJuego::FIN_PARTIDA;
    else
        estado_ = EstadoJuego::ESPERANDO_ROBO;
}

void Juego::reponerMontonDesdeDescarte() {
    if (!monton_.empty()) return;
    if (descarte_.size() <= 1) return;

    CartaRobada cartaVisible = descarte_.top();
    descarte_.pop();

    std::vector<CartaRobada> cartas;
    cartas.reserve(descarte_.size());
    while (!descarte_.empty())
    {
        cartas.push_back(descarte_.top());
        descarte_.pop();
    }

    static std::mt19937 gen(std::random_device{}());
    std::shuffle(cartas.begin(), cartas.end(), gen);

    for (const CartaRobada& c : cartas)
        monton_.push(c);

    descarte_.push(cartaVisible);
    huboRebarajeUltimoRobo_ = true;
}